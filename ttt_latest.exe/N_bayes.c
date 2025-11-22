
// N_bayes.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>

typedef enum { EMPTY=0, X=1, O=2 } Cell;
int find_blocking_move_against_ai(Cell b[3][3], Cell aiPiece);


// --- NB model ---
typedef struct {
    int counts[2][9][3];  // class (0=neg,1=pos) x feature x token (b,x,o)
    int classCount[2];
    int totalRows;
    int trained;
} NBModel;

static NBModel nb = {0};

static int Tok(const char* s){ return (s[0]=='b')?0:(s[0]=='x')?1:2; } // b,x,o
static int Lab(const char* s){ return (s[0]=='p')?1:0; }              // positive means X wins

void nb_train_from_file(const char* path){
    FILE* f=fopen(path,"r");
    if (!f) {
    SDL_Init(SDL_INIT_VIDEO); // ensure SDL ready for message box

    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "Missing AI Data File",
        "Error: Required file 'tic-tac-toe.data' is missing.\n"
        "Please place it in the same folder as ttt.exe.",
        NULL
    );

    exit(EXIT_FAILURE);  // stop the entire program
}
    char line[256];
    while(fgets(line,sizeof line,f)){
        char *sp=NULL; int feat[9]; int ok=1;
        for(int i=0;i<9;i++){ char* t=(i==0)? strtok_r(line,",\r\n",&sp):strtok_r(NULL,",\r\n",&sp); if(!t){ok=0;break;} feat[i]=Tok(t); }
        char* lbl= ok? strtok_r(NULL,",\r\n",&sp):NULL; if(!lbl) continue;
        int cls=Lab(lbl); nb.classCount[cls]++; nb.totalRows++;
        for(int i=0;i<9;i++) nb.counts[cls][i][feat[i]]++;
    }
    fclose(f); nb.trained=1;
}

static double nb_predict_logprob(int feat[9], int cls){
    // Laplace smoothing
    double logp=0.0;
    double prior=(nb.classCount[cls]+1.0)/(nb.totalRows+2.0);
    logp += log(prior);
    for(int i=0;i<9;i++){
        double num = nb.counts[cls][i][feat[i]] + 1.0;
        double den = nb.classCount[cls] + 3.0;
        logp += log(num/den);
    }
    return logp;
}

// public: P(X wins | board)
static double nb_predict_prob_xwin(int feat[9]){
    if(!nb.trained || nb.totalRows==0) return 0.5;
    double lp1 = nb_predict_logprob(feat, 1); // positive (X wins)
    double lp0 = nb_predict_logprob(feat, 0); // negative
    double m = (lp1>lp0)? lp1:lp0;
    double a = exp(lp1-m), b=exp(lp0-m);
    return a/(a+b);
}

// Helper used by both easy AI and wrapper
double prob_x_wins_after_move(Cell b[3][3], int r, int c, Cell who){
    Cell t[3][3]; for(int i=0;i<3;i++) for(int j=0;j<3;j++) t[i][j]=b[i][j];
    t[r][c]=who;
    int feat[9];
    for(int i=0;i<3;i++) for(int j=0;j<3;j++){
        Cell v=t[i][j]; feat[i*3+j]=(v==EMPTY)?0: (v==X)?1:2;
    }
    return nb_predict_prob_xwin(feat);
}

// Easy AI: assume AI is O → choose move minimizing P(X wins)
int bestMove_naive_bayes(Cell b[3][3]){
    int best=-1, ties[9], tn=0; double bestScore=1e9;
    for(int r=0;r<3;r++) for(int c=0;c<3;c++) if (b[r][c]==EMPTY){
        double p = prob_x_wins_after_move(b,r,c,O);
        if (p < bestScore - 1e-9){ bestScore=p; best=r*3+c; tn=0; ties[tn++]=best; }
        else if (fabs(p - bestScore) < 1e-9){ ties[tn++]=r*3+c; }
    }
    if (tn>0) best = ties[rand()%tn];
    return best;
}

// Wrapper: allow AI to be X or O
int bestMove_naive_bayes_for(Cell b[3][3], Cell aiPiece){
    if (aiPiece == O) return bestMove_naive_bayes(b);
    // aiPiece == X: maximize P(X wins after placing X)
    int best=-1, ties[9], tn=0; double bestScore=-1.0;
    for(int r=0;r<3;r++) for(int c=0;c<3;c++) if (b[r][c]==EMPTY){
        double p = prob_x_wins_after_move(b,r,c,X);
        if (p > bestScore + 1e-9){ bestScore=p; best=r*3+c; tn=0; ties[tn++]=best; }
        else if (fabs(p - bestScore) < 1e-9){ ties[tn++]=r*3+c; }
    }
    if (tn>0) best = ties[rand()%tn];
    return best;
}

// Detect a "one move away" win for the AI and return the cell
// where the *human* should play to block it.
// Returns index 0..8 (r*3 + c) or -1 if no immediate threat.
int find_blocking_move_against_ai(Cell b[3][3], Cell aiPiece)
{
    if (aiPiece != X && aiPiece != O) return -1;

    Cell humanPiece = (aiPiece == X) ? O : X;

    // All 8 winning lines as flat indices
    int lines[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},    // rows
        {0,3,6}, {1,4,7}, {2,5,8},    // cols
        {0,4,8}, {2,4,6}              // diagonals
    };

    for (int i = 0; i < 8; ++i) {
        int aiCount = 0;
        int humanCount = 0;
        int emptyIndex = -1;

        for (int j = 0; j < 3; ++j) {
            int idx = lines[i][j];
            int r = idx / 3;
            int c = idx % 3;

            if (b[r][c] == aiPiece) {
                aiCount++;
            } else if (b[r][c] == humanPiece) {
                humanCount++;
            } else if (b[r][c] == EMPTY) {
                emptyIndex = idx;
            }
        }

        // Threat: AI has 2 in a line, one empty, and no human piece in that line
        if (aiCount == 2 && humanCount == 0 && emptyIndex != -1) {
            return emptyIndex;   // This is where the human MUST play to block
        }
    }

    return -1;  // no immediate AI win
}


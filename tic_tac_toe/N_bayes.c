
// N_bayes.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef enum { EMPTY=0, X=1, O=2 } Cell;

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
    if(!f){ fprintf(stderr,"[NB] Warning: %s not found; using uniform priors.\n", path); nb.trained=1; return; }
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

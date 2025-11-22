
// Minimax.c
#include <stdlib.h>

typedef enum { EMPTY=0, X=1, O=2 } Cell;

// --- Internal helpers ---
static int winMinimax(int b[9]) {
    for (int i=0;i<3;i++) {
        if (b[i*3] && b[i*3]==b[i*3+1] && b[i*3+1]==b[i*3+2]) return b[i*3];
        if (b[i] && b[i]==b[i+3] && b[i+3]==b[i+6]) return b[i];
    }
    if (b[0] && b[0]==b[4] && b[4]==b[8]) return b[0];
    if (b[2] && b[2]==b[4] && b[4]==b[6]) return b[2];
    return 0;
}

static int minimax_inner(int b[9], int player) {
    int w = winMinimax(b);
    if (w) return w*player;
    int move=-1, score=-2;
    for (int i=0;i<9;i++) if (b[i]==0) {
        b[i]=player; int thisScore = -minimax_inner(b, -player); b[i]=0;
        if (thisScore>score) { score=thisScore; move=i; }
    }
    return (move==-1)?0:score;
}

// depthLimit and blunderPct are simple knobs; minimax is still full-depth perfect when depthLimit<0.
int bestMove_minimax(Cell board[3][3], int depthLimit, int blunderPct) {
    int arr[9], move=-1, best=-999; int emptyCount=0;
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) {
        int k=i*3+j;
        arr[k]=(board[i][j]==X)?1: (board[i][j]==O)?-1:0;
        if(arr[k]==0) emptyCount++;
    }
    if (emptyCount>=8 && board[1][1]==EMPTY) return 4; // open with center

    for (int i=0;i<9;i++) if (arr[i]==0) {
        arr[i]=-1; // O's move
        int sc = -minimax_inner(arr, +1);
        arr[i]=0;

        // crude difficulty knobs
        if (depthLimit>=0 && depthLimit<=3) {
            if ((rand()%100) < blunderPct) sc -= 3;
        }
        if (sc>best){best=sc; move=i;}
    }
    return move;
}

// Make minimax play as either piece by swapping symbols when AI is X.
int bestMove_minimax_for(Cell board[3][3], Cell aiPiece, int depthLimit, int blunderPct) {
    if (aiPiece == O) {
        return bestMove_minimax(board, depthLimit, blunderPct);
    }
    // aiPiece == X: swap X<->O on a copy and reuse existing logic
    Cell swp[3][3];
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++) {
            Cell v = board[i][j];
            swp[i][j] = (v==X)? O : (v==O)? X : EMPTY;
        }
    return bestMove_minimax(swp, depthLimit, blunderPct);
}

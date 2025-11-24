// Minimax.c
#include <stdlib.h>

// Define cell states for the board
typedef enum { EMPTY=0, X=1, O=2 } Cell;

// check if someone has won
static int winMinimax(int b[9]) {
    // check rows
    for (int i=0;i<3;i++) {
        if (b[i*3] && b[i*3]==b[i*3+1] && b[i*3+1]==b[i*3+2]) return b[i*3];
        if (b[i]    && b[i]==b[i+3]    && b[i+3]==b[i+6])    return b[i];
    }
    // check diagonals
    if (b[0] && b[0]==b[4] && b[4]==b[8]) return b[0];
    if (b[2] && b[2]==b[4] && b[4]==b[6]) return b[2];
    return 0; // no winner
}

// alpha beta pruning minimax
static int minimax_inner_ab(int b[9], int player, int alpha, int beta)
{
    int w = winMinimax(b);
    if (w) return w * player;   // terminal score if someone has won

    int best = -2;              // best score found so far
    int moveFound = 0;          // flag to check if any moves are possible

    // try all possible moves
    for (int i = 0; i < 9; i++) {
        if (b[i] == 0) {        // empty square
            moveFound = 1;
            b[i] = player;      // make move

            // recursive call with roles swapped
            int score = -minimax_inner_ab(b, -player, -beta, -alpha);
            b[i] = 0;           // undo move

            if (score > best) best = score;   // update best score
            if (best > alpha) alpha = best;   // update alpha

            if (alpha >= beta) break;         // prune branch
        }
    }

    if (!moveFound) return 0;   // draw if no moves left
    return best;                // return best score
}

// choose best move for O using minimax
int bestMove_minimax(Cell board[3][3], int depthLimit, int blunderPct) {
    int arr[9], move = -1, best = -999, emptyCount = 0;

    // Convert board to 1D int array
    // X is represented as +1, O as -1, empty as 0
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) {
        int k = i*3 + j;
        arr[k] = (board[i][j] == X) ? 1 :
                 (board[i][j] == O) ? -1 : 0;
        if (arr[k] == 0) emptyCount++;
    }

    // take center if possible on first move
    if (emptyCount >= 8 && board[1][1] == EMPTY)
        return 4; // index of center square

    // Try every legal move for O (O = -1)
    for (int i = 0; i < 9; i++) if (arr[i] == 0) {
        arr[i] = -1; // simulate O move

        // evaluate move using minimax
        int sc = -minimax_inner_ab(arr, +1, -2, +2);
        arr[i] = 0; // undo move

        // apply blunder chance for difficulty adjustment
        if (depthLimit >= 0 && depthLimit <= 3) {
            if ((rand() % 100) < blunderPct)
                sc -= 3; // reduce score to simulate mistake
        }

        // update best move if score is higher
        if (sc > best) {
            best = sc;
            move = i;
        }
    }

    return move; // return best move index
}

// allow minimax to play as X or O
int bestMove_minimax_for(Cell board[3][3], Cell aiPiece,
                         int depthLimit, int blunderPct)
{
    // if minimax is O, use normal routine
    if (aiPiece == O) {
        return bestMove_minimax(board, depthLimit, blunderPct);
    }

    // minimax is X: swap X<->O and reuse normal routine
    Cell swp[3][3];

    // flip board representation so solver can still assume minimax is O
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++) {
            Cell v = board[i][j];
            swp[i][j] = (v == X) ? O :
                        (v == O) ? X : EMPTY;
        }

    // run solver on swapped board
    return bestMove_minimax(swp, depthLimit, blunderPct);
}

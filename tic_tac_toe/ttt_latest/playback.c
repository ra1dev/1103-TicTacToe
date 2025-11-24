// playback.c - store and reconstruct the last Tic-Tac-Toe game

#include <stddef.h>

// This enum must match the one in main.c
typedef enum { EMPTY=0, X=1, O=2 } Cell;

typedef struct {
    int row;
    int col;
    Cell piece;
} Move;

// Record current ongoing game
static Move currentMoves[9];
static int  currentMoveCount = 0;

// Snapshot of last completed game
static Move lastMoves[9];
static int  lastMoveCount = 0;

// Called whenever a NEW game starts (board cleared for a new round)
void playback_begin_new_game(void)
{
    currentMoveCount = 0;
}

// Called whenever a move is successfully placed on the board
void playback_record_move(int row, int col, Cell piece)
{
    if (currentMoveCount >= 9)
        return;

    currentMoves[currentMoveCount].row   = row;
    currentMoves[currentMoveCount].col   = col;
    currentMoves[currentMoveCount].piece = piece;
    currentMoveCount++;
}

// Called when a game ends (win or draw)
void playback_finalize_game(void)
{
    lastMoveCount = currentMoveCount;
    for (int i = 0; i < lastMoveCount; ++i) {
        lastMoves[i] = currentMoves[i];
    }
}

// Returns non-zero if we have a completed (or at least played) game stored
int playback_has_last_game(void)
{
    return (lastMoveCount > 0);
}

// Number of moves in the last game
int playback_get_move_count(void)
{
    return lastMoveCount;
}

// Build board state at given step into outBoard.
// step = 0 => empty board
// step = k (1..lastMoveCount) => first k moves applied
void playback_build_board_at_step(int step, Cell outBoard[3][3])
{
    if (!outBoard)
        return;

    // Clear board
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            outBoard[r][c] = EMPTY;

    if (step < 0)
        step = 0;
    if (step > lastMoveCount)
        step = lastMoveCount;

    for (int i = 0; i < step; ++i) {
        int r = lastMoves[i].row;
        int c = lastMoves[i].col;
        Cell p = lastMoves[i].piece;
        if (r >= 0 && r < 3 && c >= 0 && c < 3) {
            outBoard[r][c] = p;
        }
    }
}

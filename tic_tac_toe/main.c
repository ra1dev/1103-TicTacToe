
// main_side_select.c
// SDL2 Tic-Tac-Toe with Minimax/Naive Bayes
// Flow: Start → Mode → Difficulty → Side Select
// Build (UCRT64):
//   gcc main_side_select.c Minimax.c N_bayes.c -o ttt.exe \
//     -IC:/msys64/ucrt64/include/SDL2 -LC:/msys64/ucrt64/lib \
//     -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -mwindows

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define CELL_SIZE 200

typedef enum { EMPTY=0, X=1, O=2 } Cell;

// ---- externs from your algorithm modules ----
int bestMove_minimax(Cell b[3][3], int depthLimit, int blunderPct);
int bestMove_minimax_for(Cell b[3][3], Cell aiPiece, int depthLimit, int blunderPct);
int bestMove_naive_bayes(Cell b[3][3]);
int bestMove_naive_bayes_for(Cell b[3][3], Cell aiPiece);
void nb_train_from_file(const char* path);

// ---- global state ----
Cell board[3][3];

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

typedef enum { MODE_MP=1, MODE_SP=2 } GameMode;
typedef enum { DIFF_EASY=0, DIFF_MEDIUM=1, DIFF_HARD=2 } Difficulty;
typedef enum { SIDE_X=0, SIDE_O=1 } PlayerSide;

int currentPlayer = 1;          // 1 = Player X turn, 2 = Player O turn
GameMode gameMode = MODE_SP;    // default
Difficulty aiDiff = DIFF_EASY;  // default for SP
PlayerSide playerSide = SIDE_X; // default: human plays X
Cell aiPiece = O;               // derived from playerSide

int scoreX = 0, scoreO = 0, firstPlayer = 1;
int needsRedraw = 1;
SDL_Rect backButton;

SDL_Texture *xTexture = NULL;
SDL_Texture *oTexture = NULL;
SDL_Color xColor = {220, 50, 50, 255};
SDL_Color oColor = {50, 180, 220, 255};
SDL_Color textColor = {255, 255, 255, 255};

static void initBoard(void) {
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) board[i][j] = EMPTY;
}

static SDL_Texture* createTextTexture(const char* text, TTF_Font* fnt, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(fnt, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

static void drawScores(void) {
    char scoreText[128];
    const char* roles = (playerSide==SIDE_X) ? "You:X  AI:O" : "You:O  AI:X";
    snprintf(scoreText, sizeof(scoreText), "X:%d  O:%d  [%s]  (%s)",
             scoreX, scoreO,
             (aiDiff==DIFF_EASY)?"EASY(NB)":(aiDiff==DIFF_MEDIUM)?"MEDIUM":"HARD",
             roles);
    SDL_Texture* scoreTex = createTextTexture(scoreText, font, textColor);
    int w, h; SDL_QueryTexture(scoreTex,NULL,NULL,&w,&h);
    SDL_Rect dest = (SDL_Rect){10, 10, w, h};
    SDL_RenderCopy(renderer, scoreTex, NULL, &dest);
    SDL_DestroyTexture(scoreTex);
}

static void drawbackButton(void) {
    SDL_Texture* quitTex = createTextTexture("Back", font, textColor);
    int w,h; SDL_QueryTexture(quitTex,NULL,NULL,&w,&h);
    backButton = (SDL_Rect){WINDOW_WIDTH - w - 20, 20, w, h};
    SDL_RenderCopy(renderer, quitTex, NULL, &backButton);
    SDL_DestroyTexture(quitTex);
}

static void drawBoard(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 1; i < 3; i++) {
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, WINDOW_HEIGHT);
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, WINDOW_WIDTH, i * CELL_SIZE);
    }
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) {
        int x = j * CELL_SIZE, y = i * CELL_SIZE;
        if (board[i][j] == X || board[i][j]==O) {
            SDL_Texture* tex = (board[i][j]==X)? xTexture : oTexture;
            SDL_Rect dest = (SDL_Rect){x + 20, y + 20, CELL_SIZE - 40, CELL_SIZE - 40};
            SDL_RenderCopy(renderer, tex, NULL, &dest);
        }
    }
    drawScores();
    drawbackButton();
    SDL_RenderPresent(renderer);
    needsRedraw = 0;
}

static void updateScores(int winner) { if (winner==X) scoreX++; else if (winner==O) scoreO++; }

static int checkWin(void) {
    for (int i=0;i<3;i++) {
        if (board[i][0]!=EMPTY && board[i][0]==board[i][1] && board[i][1]==board[i][2]) return board[i][0];
        if (board[0][i]!=EMPTY && board[0][i]==board[1][i] && board[1][i]==board[2][i]) return board[0][i];
    }
    if (board[0][0]!=EMPTY && board[0][0]==board[1][1] && board[1][1]==board[2][2]) return board[0][0];
    if (board[0][2]!=EMPTY && board[0][2]==board[1][1] && board[1][1]==board[2][0]) return board[0][2];
    return 0;
}

static int isBoardFull(void) { for(int i=0;i<3;i++) for(int j=0;j<3;j++) if(board[i][j]==EMPTY) return 0; return 1; }

// ---------- Menus ----------
static int modeMenu(void) {
    SDL_Event event;
    for(;;) {
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderClear(renderer);
        SDL_Texture* t1=createTextTexture("Single Player",font,textColor);
        SDL_Texture* t2=createTextTexture("Multiplayer",font,textColor);
        int w1,h1,w2,h2; SDL_QueryTexture(t1,NULL,NULL,&w1,&h1); SDL_QueryTexture(t2,NULL,NULL,&w2,&h2);
        SDL_Rect r1={WINDOW_WIDTH/2-w1/2, WINDOW_HEIGHT/3-h1/2, w1,h1};
        SDL_Rect r2={WINDOW_WIDTH/2-w2/2, WINDOW_HEIGHT*2/3-h2/2, w2,h2};
        SDL_RenderCopy(renderer,t1,NULL,&r1); SDL_RenderCopy(renderer,t2,NULL,&r2); SDL_RenderPresent(renderer);
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) { SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return 0; }
            if(event.type==SDL_MOUSEBUTTONDOWN){
                int mx=event.button.x,my=event.button.y;
                if(mx>=r1.x && mx<=r1.x+r1.w && my>=r1.y && my<=r1.y+r1.h) { SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return MODE_SP; }
                if(mx>=r2.x && mx<=r2.x+r2.w && my>=r2.y && my<=r2.y+r2.h) { SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return MODE_MP; }
            }
        }
        SDL_DestroyTexture(t1); SDL_DestroyTexture(t2);
        SDL_Delay(16);
    }
}

static Difficulty difficultyMenu(void) {
    SDL_Event event;
    for(;;){
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderClear(renderer);
        SDL_Texture* t0=createTextTexture("Select Difficulty",font,textColor);
        SDL_Texture* t1=createTextTexture("Easy (Naive Bayes)",font,textColor);
        SDL_Texture* t2=createTextTexture("Medium (Minimax)",font,textColor);
        SDL_Texture* t3=createTextTexture("Hard (Perfect Minimax)",font,textColor);
        int w0,h0,w1,h1,w2,h2,w3,h3;
        SDL_QueryTexture(t0,NULL,NULL,&w0,&h0);
        SDL_QueryTexture(t1,NULL,NULL,&w1,&h1);
        SDL_QueryTexture(t2,NULL,NULL,&w2,&h2);
        SDL_QueryTexture(t3,NULL,NULL,&w3,&h3);
        SDL_Rect r0={WINDOW_WIDTH/2-w0/2, 80, w0,h0};
        SDL_Rect r1={WINDOW_WIDTH/2-w1/2, 160, w1,h1};
        SDL_Rect r2={WINDOW_WIDTH/2-w2/2, 220, w2,h2};
        SDL_Rect r3={WINDOW_WIDTH/2-w3/2, 280, w3,h3};
        SDL_RenderCopy(renderer,t0,NULL,&r0);
        SDL_RenderCopy(renderer,t1,NULL,&r1);
        SDL_RenderCopy(renderer,t2,NULL,&r2);
        SDL_RenderCopy(renderer,t3,NULL,&r3);
        SDL_RenderPresent(renderer);
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) { SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3); return DIFF_EASY; }
            if(event.type==SDL_MOUSEBUTTONDOWN){
                int mx=event.button.x,my=event.button.y;
                if(mx>=r1.x && mx<=r1.x+r1.w && my>=r1.y && my<=r1.y+r1.h) { SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3); return DIFF_EASY; }
                if(mx>=r2.x && mx<=r2.x+r2.w && my>=r2.y && my<=r2.y+r2.h) { SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3); return DIFF_MEDIUM; }
                if(mx>=r3.x && mx<=r3.x+r3.w && my>=r3.y && my<=r3.y+r3.h) { SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3); return DIFF_HARD; }
            }
        }
        SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); SDL_DestroyTexture(t3);
        SDL_Delay(16);
    }
}

static PlayerSide sideMenu(void) {
    SDL_Event event;
    for(;;){
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderClear(renderer);
        SDL_Texture* t0=createTextTexture("Choose Your Side",font,textColor);
        SDL_Texture* t1=createTextTexture("Play as X",font,textColor);
        SDL_Texture* t2=createTextTexture("Play as O",font,textColor);
        int w0,h0,w1,h1,w2,h2;
        SDL_QueryTexture(t0,NULL,NULL,&w0,&h0);
        SDL_QueryTexture(t1,NULL,NULL,&w1,&h1);
        SDL_QueryTexture(t2,NULL,NULL,&w2,&h2);
        SDL_Rect r0={WINDOW_WIDTH/2-w0/2, 80, w0,h0};
        SDL_Rect r1={WINDOW_WIDTH/2-w1/2, 160, w1,h1};
        SDL_Rect r2={WINDOW_WIDTH/2-w2/2, 220, w2,h2};
        SDL_RenderCopy(renderer,t0,NULL,&r0);
        SDL_RenderCopy(renderer,t1,NULL,&r1);
        SDL_RenderCopy(renderer,t2,NULL,&r2);
        SDL_RenderPresent(renderer);
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) { SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return SIDE_X; }
            if(event.type==SDL_MOUSEBUTTONDOWN){
                int mx=event.button.x,my=event.button.y;
                if(mx>=r1.x&&mx<=r1.x+r1.w&&my>=r1.y&&my<=r1.y+r1.h){ SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return SIDE_X; }
                if(mx>=r2.x&&mx<=r2.x+r2.w&&my>=r2.y&&my<=r2.y+r2.h){ SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2); return SIDE_O; }
            }
        }
        SDL_DestroyTexture(t0); SDL_DestroyTexture(t1); SDL_DestroyTexture(t2);
        SDL_Delay(16);
    }
}

// ---------- Messages ----------
static void displayMessage(const char* message) {
    SDL_Texture* msgTex=createTextTexture(message,font,textColor);
    int w,h; SDL_QueryTexture(msgTex,NULL,NULL,&w,&h);
    SDL_Rect dest={WINDOW_WIDTH/2-w/2, WINDOW_HEIGHT/2-h/2, w,h};
    SDL_RenderCopy(renderer,msgTex,NULL,&dest);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(msgTex);
    SDL_Delay(1200);
}

// ---------- AI move dispatch using chosen side ----------
static void botMove(void) {
    int move = -1;
    if (aiDiff == DIFF_EASY) {
        move = bestMove_naive_bayes_for(board, aiPiece);
    } else if (aiDiff == DIFF_MEDIUM) {
        move = bestMove_minimax_for(board, aiPiece, 3 /*depth proxy*/, 20 /*blunder%*/);
    } else {
        move = bestMove_minimax_for(board, aiPiece, -1, 0); // hard: perfect
    }
    if (move != -1) {
        int i = move / 3, j = move % 3;
        board[i][j] = aiPiece;
        needsRedraw = 1;
    }
}

// ===================== MAIN =====================
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return 1; }
    if (TTF_Init() == -1) { fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError()); SDL_Quit(); return 1; }

    window = SDL_CreateWindow("Tic Tac Toe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) { fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); return 1; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError()); return 1; }
    font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 32);
    if (!font) font = TTF_OpenFont("/System/Library/Fonts/SFNS.ttf", 24);
    if (!font) { fprintf(stderr, "Could not open a font.\n"); return 1; }

    xTexture = createTextTexture("X", font, xColor);
    oTexture = createTextTexture("O", font, oColor);

    // Train NB (file must be in same folder)
    nb_train_from_file("tic-tac-toe.data");

    // Start → Mode → Difficulty → Side
    int modeSel = modeMenu();
    if (modeSel==0) { goto cleanup; }
    gameMode = (modeSel==MODE_SP)? MODE_SP: MODE_MP;
    if (gameMode==MODE_SP) {
        aiDiff = difficultyMenu();
        playerSide = sideMenu();
        aiPiece = (playerSide==SIDE_X) ? O : X;
    }

    int running = 1;
    initBoard();
    currentPlayer = firstPlayer; // 1:X, 2:O

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running=0;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x, my = event.button.y;
                if (mx >= backButton.x && mx <= backButton.x + backButton.w &&
                    my >= backButton.y && my <= backButton.y + backButton.h) {
                    // Back to menus
                    int newMode = modeMenu(); if (newMode==0) { running=0; break; }
                    gameMode = (newMode==MODE_SP)? MODE_SP: MODE_MP;
                    if (gameMode==MODE_SP) {
                        aiDiff = difficultyMenu();
                        playerSide = sideMenu();
                        aiPiece = (playerSide==SIDE_X) ? O : X;
                    }
                    scoreX=scoreO=0; initBoard(); currentPlayer=firstPlayer; needsRedraw=1; continue;
                }
                // grid click
                int x = mx / CELL_SIZE, y = my / CELL_SIZE;
                if (x>=0&&x<3&&y>=0&&y<3 && board[y][x]==EMPTY) {
                    Cell playerPiece = (gameMode==MODE_SP)
                                        ? ((playerSide==SIDE_X) ? X : O)
                                        : ((currentPlayer==1)?X:O);
                    board[y][x] = playerPiece;
                    currentPlayer = (currentPlayer == 1) ? 2 : 1;
                    needsRedraw = 1;
                }
            }
        }

        // AI turn
        if (gameMode==MODE_SP) {
            int whoseTurnPiece = (currentPlayer==1)? X : O;
            if (whoseTurnPiece == aiPiece && !isBoardFull() && checkWin()==0) {
                botMove();
                currentPlayer = (currentPlayer == 1) ? 2 : 1;
            }
        }

        if (needsRedraw) drawBoard();
        SDL_Delay(16);

        int winner = checkWin();
        if (winner || isBoardFull()) {
            updateScores(winner);
            if (winner == X) displayMessage(gameMode==MODE_SP ? ((playerSide==SIDE_X) ? "You Win!" : "Computer (X) Wins!") : "Player X Wins!");
            else if (winner == O) displayMessage(gameMode==MODE_SP ? ((playerSide==SIDE_O) ? "You Win!" : "Computer (O) Wins!") : "Player O Wins!");
            else displayMessage("Draw!");
            if (winner == X) firstPlayer = 2; else if (winner == O) firstPlayer = 1;
            currentPlayer = firstPlayer; initBoard(); needsRedraw=1;
        }
    }

cleanup:
    SDL_DestroyTexture(xTexture); SDL_DestroyTexture(oTexture);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit();
    return 0;
}

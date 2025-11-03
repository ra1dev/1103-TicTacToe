#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define CELL_SIZE 200

typedef enum { EMPTY, X, O } Cell;
Cell board[3][3];

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

int currentPlayer = 1;
int singlePlayerMode = 0;
int scoreX = 0;
int scoreO = 0;
int firstPlayer = 1; // Who starts each round
int minimax(int b[9], int player);

SDL_Texture *xTexture = NULL;
SDL_Texture *oTexture = NULL;

SDL_Color xColor = {220, 50, 50, 255};
SDL_Color oColor = {50, 180, 220, 255};
SDL_Color textColor = {255, 255, 255, 255};

int needsRedraw = 1;
SDL_Rect backButton;

void initBoard() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            board[i][j] = EMPTY;
}

SDL_Texture* createTextTexture(const char* text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void drawScores() {
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "X: %d   O: %d", scoreX, scoreO);
    SDL_Texture* scoreTex = createTextTexture(scoreText, font, textColor);
    int w, h;
    SDL_QueryTexture(scoreTex, NULL, NULL, &w, &h);
    SDL_Rect dest = {10, 10, w, h};
    SDL_RenderCopy(renderer, scoreTex, NULL, &dest);
    SDL_DestroyTexture(scoreTex);
}

void drawbackButton() {
    SDL_Texture* quitTex = createTextTexture("Back", font, textColor);
    int w, h;
    SDL_QueryTexture(quitTex, NULL, NULL, &w, &h);
    backButton = (SDL_Rect){WINDOW_WIDTH - w - 20, 20, w, h};
    SDL_RenderCopy(renderer, quitTex, NULL, &backButton);
    SDL_DestroyTexture(quitTex);
}

void drawBoard() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 1; i < 3; i++) {
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, WINDOW_HEIGHT);
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, WINDOW_WIDTH, i * CELL_SIZE);
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int x = j * CELL_SIZE;
            int y = i * CELL_SIZE;
            if (board[i][j] == X) {
                SDL_Rect dest = {x + 20, y + 20, CELL_SIZE - 40, CELL_SIZE - 40};
                SDL_RenderCopy(renderer, xTexture, NULL, &dest);
            } else if (board[i][j] == O) {
                SDL_Rect dest = {x + 20, y + 20, CELL_SIZE - 40, CELL_SIZE - 40};
                SDL_RenderCopy(renderer, oTexture, NULL, &dest);
            }
        }
    }
    drawScores();
    drawbackButton();
    SDL_RenderPresent(renderer);
    needsRedraw = 0;
}

void updateScores(int winner) {
    if (winner == X) scoreX++;
    else if (winner == O) scoreO++;
}

int checkWin() {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != EMPTY && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0];
        if (board[0][i] != EMPTY && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i];
    }
    if (board[0][0] != EMPTY && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0];
    if (board[0][2] != EMPTY && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2];
    return 0;
}

int isBoardFull() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == EMPTY)
                return 0;
    return 1;
}

int bestMove(Cell board[3][3]) {
    int b[9];
    // Convert 3x3 board to 1D array
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            b[i*3 + j] = (board[i][j] == X) ? 1 : (board[i][j] == O) ? -1 : 0;

    int move = -1;
    int score = -2;
    for (int i = 0; i < 9; i++) {
        if (b[i] == 0) {
            b[i] = -1; // Computer is O = -1
            int thisScore = -minimax(b, 1);
            if (thisScore > score) {
                score = thisScore;
                move = i;
            }
            b[i] = 0;
        }
    }
    return move;
}

int winMinimax(int b[9]) {
    for (int i = 0; i < 3; i++) {
        if (b[i*3] != 0 && b[i*3] == b[i*3+1] && b[i*3+1] == b[i*3+2])
            return b[i*3];
        if (b[i] != 0 && b[i] == b[i+3] && b[i+3] == b[i+6])
            return b[i];
    }
    if (b[0] != 0 && b[0] == b[4] && b[4] == b[8]) return b[0];
    if (b[2] != 0 && b[2] == b[4] && b[4] == b[6]) return b[2];
    return 0;
}

int minimax(int b[9], int player) {
    int winner = winMinimax(b);
    if (winner != 0) return winner * player;

    int move = -1;
    int score = -2; // Losing moves are worse
    for (int i = 0; i < 9; i++) {
        if (b[i] == 0) {
            b[i] = player;
            int thisScore = -minimax(b, -player);
            if (thisScore > score) {
                score = thisScore;
                move = i;
            }
            b[i] = 0;
        }
    }
    if (move == -1) return 0;
    return score;
}

void botMove() {
    int move = bestMove(board);
    if (move != -1) {
        int i = move / 3;
        int j = move % 3;
        board[i][j] = O;
        needsRedraw = 1;
    }
}

int modeMenu() {
    SDL_Event event;
    int running = 1;

    while (running) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Texture* computerText = createTextTexture("Singleplayer", font, textColor);
        SDL_Texture* multiplayerText = createTextTexture("Multiplayer", font, textColor);

        int w, h;
        SDL_QueryTexture(computerText, NULL, NULL, &w, &h);
        SDL_Rect compRect = {WINDOW_WIDTH/2 - w/2, WINDOW_HEIGHT/3 - h/2, w, h};
        SDL_QueryTexture(multiplayerText, NULL, NULL, &w, &h);
        SDL_Rect multiRect = {WINDOW_WIDTH/2 - w/2, WINDOW_HEIGHT*2/3 - h/2, w, h};

        SDL_RenderCopy(renderer, computerText, NULL, &compRect);
        SDL_RenderCopy(renderer, multiplayerText, NULL, &multiRect);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            // if (event.type == SDL_QUIT) return 0;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;
                if (mx >= multiRect.x && mx <= multiRect.x + multiRect.w && my >= multiRect.y && my <= multiRect.y + multiRect.h)
                    return 1; //multiplayer (singleplayermode = 1)
                if (mx >= compRect.x && mx <= compRect.x + compRect.w && my >= compRect.y && my <= compRect.y + compRect.h)
                    return 2; //singleplayer (singleplayermode = 2)
            }
        }
        SDL_DestroyTexture(multiplayerText);
        SDL_DestroyTexture(computerText);
        SDL_Delay(16);
    }
    return 0;
}

void displayMessage(const char* message) {
    SDL_Texture* msgTex = createTextTexture(message, font, textColor);
    int w, h;
    SDL_QueryTexture(msgTex, NULL, NULL, &w, &h);
    SDL_Rect dest = {WINDOW_WIDTH/2 - w/2, WINDOW_HEIGHT/2 - h/2, w, h};
    SDL_RenderCopy(renderer, msgTex, NULL, &dest);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(msgTex);
    SDL_Delay(2000);
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    window = SDL_CreateWindow("Tic Tac Toe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 32);

    xTexture = createTextTexture("X", font, xColor);
    oTexture = createTextTexture("O", font, oColor);

    singlePlayerMode = modeMenu();
    // if (!singlePlayerMode) { SDL_Quit(); return 0; }

    int running = 1;
    initBoard();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;
                if (mx >= backButton.x && mx <= backButton.x + backButton.w &&
                    my >= backButton.y && my <= backButton.y + backButton.h) {
                    // running = 0;
                    singlePlayerMode = modeMenu();  // reselect mode properly
                    currentPlayer = firstPlayer;    // reset who starts
                    initBoard();
                    needsRedraw = 1;

                } else {
                    int x = mx / CELL_SIZE;
                    int y = my / CELL_SIZE;
                    if (board[y][x] == EMPTY) {
                        board[y][x] = (currentPlayer == 1) ? X : O;
                        currentPlayer = (currentPlayer == 1) ? 2 : 1;
                        needsRedraw = 1;
                    }
                }
            }
        }

        if (singlePlayerMode == 2 && currentPlayer == 2 && !isBoardFull() && checkWin() == 0) {
            botMove();
            currentPlayer = 1;
        }

        if (needsRedraw) drawBoard();
        SDL_Delay(16);

        int winner = checkWin();
        if (winner || isBoardFull()) {
            updateScores(winner);

            if (winner == X) displayMessage("Player X Wins!");
            else if (winner == O) displayMessage(singlePlayerMode == 2 ? "Computer Wins!" : "Player O Wins!");
            else displayMessage("Draw!");

            // Loser starts next round
            if (winner == X) firstPlayer = 2;
            else if (winner == O) firstPlayer = 1;
            // Draw → keep same starter

            currentPlayer = firstPlayer;
            initBoard();
            needsRedraw = 1;
        }
    }

    SDL_DestroyTexture(xTexture);
    SDL_DestroyTexture(oTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
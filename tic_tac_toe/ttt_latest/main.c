// main.c — SDL2 Tic-Tac-Toe
// Build (UCRT64):
//   gcc main.c Minimax.c N_bayes.c -o ttt.exe \
//     -IC:/msys64/ucrt64/include/SDL2 -LC:/msys64/ucrt64/lib \
//     -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm -mwindows

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Window / board sizing
#define WINDOW_WIDTH   600
#define WINDOW_HEIGHT  840

#define CELL_SIZE      140
#define GRID_GAP       12
#define BOARD_PAD      18

#define PADDING_TOP                16
#define MODE_BOTTOM_PAD            24
#define SCOREBOXES_BOTTOM_PAD      26
#define TURN_LABEL_BOTTOM_PAD      18
#define BOARD_BOTTOM_PAD           36
#define WINLINE_THICKNESS          8

// Game types
typedef enum { EMPTY=0, X=1, O=2 } Cell;
typedef enum { MODE_MP=1, MODE_SP=2 } GameMode;
typedef enum { DIFF_BACK=-1, DIFF_EASY=0, DIFF_MEDIUM=1, DIFF_HARD=2 } Difficulty;
typedef enum { SIDE_X=0, SIDE_O=1 } PlayerSide;
typedef enum { ICON_NONE=0, ICON_SOLO=1, ICON_DUO=2 } ButtonIcon;
typedef enum { THEME_DARK=0, THEME_FUN=1 } Theme;

// function prototypes

//decision-making logic for AI
//hint for user to block AI from winning
int find_blocking_move_against_ai(Cell b[3][3], Cell aiPiece); 

//depthLimit for how far the AI searches
//blunderPct for making suboptimal moves to simulate human error
// int bestMove_minimax(Cell b[3][3], int depthLimit, int blunderPct);
int bestMove_minimax_for(Cell b[3][3], Cell aiPiece, int depthLimit, int blunderPct); // aiPiece is the AI's chosen piece

// int bestMove_naive_bayes(Cell b[3][3]); // best move based on trained data
int bestMove_naive_bayes_for(Cell b[3][3], Cell aiPiece); // best move based on trained data, but for AI's piece
void nb_train_from_file(const char* path);  // references to N_bayes.c for training data

// UI-related
static void renderGame(void);   // draws game board
static Theme themeMenu(void);   // calls function that allows user to choose theme
static void playbackScreen(void);  // calls function that shows history of most recent game


// core state of SDL objects
Cell board[3][3];
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

Theme currentTheme = THEME_DARK;    // sets current theme to Dark

// HINT FUNCTION
int hintIndex = -1; // -1 = no hint, 0..8 = r*3 + c
Uint32 lastHumanActivityTicks = 0;
static const SDL_Color hintFill = { 70, 96, 140, 255 };


// PLAYBACK FUNCTION
void playback_begin_new_game(void);
void playback_record_move(int row, int col, Cell piece);
void playback_finalize_game(void);
int  playback_has_last_game(void);
int  playback_get_move_count(void);
void playback_build_board_at_step(int step, Cell outBoard[3][3]);


// PLAYBACK FUNCTION
int currentPlayer = 1;  // 1 = X, 2 = O
GameMode gameMode = MODE_SP;
Difficulty aiDiff = DIFF_EASY;
PlayerSide playerSide = SIDE_X;
Cell aiPiece = O;
int scoreX = 0, scoreO = 0;
int firstPlayer = 1;            // 1 -> X starts, 2 -> O starts

// UI state
int needsRedraw = 1;
SDL_Rect backButton = {0};
SDL_Rect resetButton = {0};
SDL_Rect boardRect  = {0};

// Colors
static const SDL_Color xColor     = { 60, 230,  90, 255}; // default X
static const SDL_Color oColor     = {250, 160, 170, 255}; // default O
static const SDL_Color textLight  = {230, 235, 255, 255};
static const SDL_Color textDark   = { 20,  24,  32, 255};
static const SDL_Color cardFill   = { 28,  36,  56, 255};
static const SDL_Color cardBorder = { 70,  80, 110, 255};
static const SDL_Color boardFill  = { 30,  42,  66, 255};
static const SDL_Color boardBorder= { 60,  78, 110, 255};
static const SDL_Color cellFill   = { 40,  54,  82, 255};
static const SDL_Color cellBorder = { 70,  86, 120, 255};

// Fun theme colours
static const SDL_Color funBoardFill  = { 255, 243, 176, 255 };
static const SDL_Color funCellFill   = { 255, 250, 210, 255 };

// Background colour based on current theme
static SDL_Color getBackgroundColor(void) {
    if (currentTheme == THEME_FUN) {
        SDL_Color c = { 255, 235, 150, 255 }; // kid friendly yellow
        return c;
    } else {
        SDL_Color c = { 0, 0, 0, 255 };       // dark
        return c;
    }
}

// Text colour for non-button text
static SDL_Color getTextColor(void) {
    if (currentTheme == THEME_FUN) {
        SDL_Color c = { 0, 0, 0, 255 }; // black text on yellow
        return c;
    } else {
        return textLight;               // light text on dark background
    }
}

// resets each cell to EMPTY constant
static void initBoard(void) {
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++)
            board[i][j] = EMPTY;
}

// text rendering utility in SDL2 with SDL2_ttf
static SDL_Texture* createTextTexture(const char* text, TTF_Font* fnt, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(fnt, text, color);
    if (!surface) return NULL;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// win-detection logic (checks rows, columns, diagonals)
static int getWinLine(int *r1, int *c1, int *r2, int *c2)
{
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != EMPTY &&
            board[i][0] == board[i][1] &&
            board[i][1] == board[i][2]) {
            *r1 = i; *c1 = 0; *r2 = i; *c2 = 2;
            return board[i][0];
        }
        if (board[0][i] != EMPTY &&
            board[0][i] == board[1][i] &&
            board[1][i] == board[2][i]) {
            *r1 = 0; *c1 = i; *r2 = 2; *c2 = i;
            return board[0][i];
        }
    }
    if (board[0][0] != EMPTY &&
        board[0][0] == board[1][1] &&
        board[1][1] == board[2][2]) {
        *r1 = 0; *c1 = 0; *r2 = 2; *c2 = 2;
        return board[0][0];
    }
    if (board[0][2] != EMPTY &&
        board[0][2] == board[1][1] &&
        board[1][1] == board[2][0]) {
        *r1 = 0; *c1 = 2; *r2 = 2; *c2 = 0;
        return board[0][2];
    }
    return 0;
}

// SDL renderer for GUI
static void drawThickLine(float x1, float y1, float x2, float y2, int thickness, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len == 0) return;
    dx /= len; dy /= len;

    float nx = -dy, ny = dx;
    int half = thickness / 2;

    for (int i = -half; i <= half; i++) {
        float ox = nx * i;
        float oy = ny * i;
        SDL_RenderDrawLine(renderer,
                           (int)(x1 + ox),
                           (int)(y1 + oy),
                           (int)(x2 + ox),
                           (int)(y2 + oy));
    }
}

// Animation of Win Line
static void animateWinLine(SDL_Rect boardRect, SDL_Color lineColor)
{
    int r1, c1, r2, c2;
    int winner = getWinLine(&r1, &c1, &r2, &c2);
    if (!winner) return;

    const int cellStep = CELL_SIZE + GRID_GAP;
    const int bx = boardRect.x + BOARD_PAD;
    const int by = boardRect.y + BOARD_PAD;

    float x1 = bx + c1 * cellStep + CELL_SIZE * 0.5f;
    float y1 = by + r1 * cellStep + CELL_SIZE * 0.5f;
    float x2 = bx + c2 * cellStep + CELL_SIZE * 0.5f;
    float y2 = by + r2 * cellStep + CELL_SIZE * 0.5f;

    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len > 0.0f) {
        float extend = 0.12f * len;
        dx /= len; dy /= len;
        x1 -= dx * extend;
        y1 -= dy * extend;
        x2 += dx * extend;
        y2 += dy * extend;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const float duration_ms = 600.0f;
    Uint32 start = SDL_GetTicks();

    // animation loop
    for (;;) {
        float t = (SDL_GetTicks() - start) / duration_ms;
        if (t > 1.0f) t = 1.0f;

        renderGame();

        float cx = x1 + (x2 - x1) * t;
        float cy = y1 + (y2 - y1) * t;

        SDL_Color c = lineColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        float ldx = x2 - x1, ldy = y2 - y1;
        float llen = sqrtf(ldx*ldx + ldy*ldy);
        float nx = 0.0f, ny = 0.0f;
        if (llen > 0.0f) {
            nx = -ldy / llen;
            ny =  ldx / llen;
        }

        int half = WINLINE_THICKNESS / 2;
        for (int i = -half; i <= half; ++i) {
            int ox = (int)roundf(nx * i);
            int oy = (int)roundf(ny * i);
            SDL_RenderDrawLine(renderer,
                               (int)(x1 + ox), (int)(y1 + oy),
                               (int)(cx + ox), (int)(cy + oy));
        }

        SDL_RenderPresent(renderer);
        if (t >= 1.0f) break;
        SDL_Delay(16);
    }

    // draw winning line that stays on page
    renderGame();
    SDL_Color c = lineColor;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    drawThickLine(x1, y1, x2, y2, WINLINE_THICKNESS, c);
    SDL_RenderPresent(renderer);
}

// SDL renderer for GUI
static void setColor(SDL_Color c){
    SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);
}

//  Render player icon
static void hline(int x1, int x2, int y)
{
    if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
    SDL_RenderDrawLine(renderer, x1, y, x2, y);
}

static void fillCircle(int cx, int cy, int r)
{
    for (int dy = -r; dy <= r; ++dy) {
        int dx = (int)(sqrt((double)r*r - (double)dy*dy) + 0.5);
        hline(cx - dx, cx + dx, cy + dy);
    }
}

static void fillRing(int cx, int cy, int outerR, int thickness,
                     SDL_Color ringColor, SDL_Color bgColorLocal)
{
    setColor(ringColor);
    fillCircle(cx, cy, outerR);

    int innerR = outerR - thickness;
    if (innerR > 0) {
        setColor(bgColorLocal);
        fillCircle(cx, cy, innerR);
    }
}

// Render button shapes
static void drawRoundedRectFilled(SDL_Rect r, int rad, SDL_Color fill)
{
    if (rad < 1) {
        setColor(fill);
        SDL_RenderFillRect(renderer, &r);
        return;
    }
    if (rad*2 > r.w) rad = r.w/2;
    if (rad*2 > r.h) rad = r.h/2;

    setColor(fill);

    SDL_Rect mid = { r.x + rad, r.y, r.w - 2*rad, r.h };
    SDL_RenderFillRect(renderer, &mid);

    SDL_Rect left = { r.x, r.y + rad, rad, r.h - 2*rad };
    SDL_Rect right= { r.x + r.w - rad, r.y + rad, rad, r.h - 2*rad };
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);

    for (int dy = -rad; dy <= rad; ++dy) {
        int dx = (int)(sqrt((double)rad*rad - (double)dy*dy) + 0.5);
        hline(r.x + rad - dx,           r.x + rad,           r.y + rad - dy);
        hline(r.x + r.w - rad,          r.x + r.w - rad + dx,r.y + rad - dy);
        hline(r.x + rad - dx,           r.x + rad,           r.y + r.h - rad + dy);
        hline(r.x + r.w - rad,          r.x + r.w - rad + dx,r.y + r.h - rad + dy);
    }
}

static void drawRoundedRectOutline(SDL_Rect r, int rad, SDL_Color border)
{
    if (rad < 1) {
        setColor(border);
        SDL_RenderDrawRect(renderer, &r);
        return;
    }
    if (rad*2 > r.w) rad = r.w/2;
    if (rad*2 > r.h) rad = r.h/2;

    setColor(border);

    SDL_RenderDrawLine(renderer, r.x + rad, r.y, r.x + r.w - rad - 1, r.y);
    SDL_RenderDrawLine(renderer, r.x + rad, r.y + r.h - 1, r.x + r.w - rad - 1, r.y + r.h - 1);
    SDL_RenderDrawLine(renderer, r.x, r.y + rad, r.x, r.y + r.h - rad - 1);
    SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y + rad, r.x + r.w - 1, r.y + r.h - rad - 1);

    for (int a = 0; a <= rad; ++a) {
        int b = (int)(sqrt((double)rad*rad - (double)a*a) + 0.5);
        SDL_RenderDrawPoint(renderer, r.x + rad - a,           r.y + rad - b);
        SDL_RenderDrawPoint(renderer, r.x + rad - b,           r.y + rad - a);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + a, r.y + rad - b);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + b, r.y + rad - a);
        SDL_RenderDrawPoint(renderer, r.x + rad - a,           r.y + r.h - rad - 1 + b);
        SDL_RenderDrawPoint(renderer, r.x + rad - b,           r.y + r.h - rad - 1 + a);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + a, r.y + r.h - rad - 1 + b);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + b, r.y + r.h - rad - 1 + a);
    }
}

// render pieces (X, O)
static void drawThickDiag(int x1, int y1, int x2, int y2, int thickness, SDL_Color color)
{
    setColor(color);
    double dx = (double)(x2 - x1);
    double dy = (double)(y2 - y1);
    double len = sqrt(dx*dx + dy*dy);
    if (len == 0) return;
    double nx = -dy / len;
    double ny =  dx / len;

    int half = thickness / 2;
    for (int t = -half; t <= half; ++t) {
        int ox = (int)round(nx * t);
        int oy = (int)round(ny * t);
        SDL_RenderDrawLine(renderer, x1+ox, y1+oy, x2+ox, y2+oy);
    }
}

static void drawXIcon(SDL_Rect cell, int inset, int thickness, SDL_Color color)
{
    int x0 = cell.x + inset;
    int y0 = cell.y + inset;
    int x1 = cell.x + cell.w - inset;
    int y1 = cell.y + cell.h - inset;

    drawThickDiag(x0, y0, x1, y1, thickness, color);
    drawThickDiag(x0, y1, x1, y0, thickness, color);
}

static void drawOIcon(SDL_Rect cell, int inset, int thickness,
                      SDL_Color color, SDL_Color bgColorLocal)
{
    int cx = cell.x + cell.w/2;
    int cy = cell.y + cell.h/2;
    int outerR = (cell.w < cell.h ? cell.w : cell.h)/2 - inset;
    if (outerR < thickness) outerR = thickness;
    fillRing(cx, cy, outerR, thickness, color, bgColorLocal);
}

// Person icons for buttons
static void drawPersonOutline(int x, int y, int size, int stroke,
                              SDL_Color col, SDL_Color bg)
{
    if (stroke < 1) stroke = 1;

    int headR   = (int)round(size * 0.28);
    int cx      = x + size/2;
    int cy      = y + (int)round(size * 0.28);
    fillRing(cx, cy, headR, stroke, col, bg);

    int barW = (int)round(size * 0.88);
    int barH = (int)round(size * 0.40);
    int barY = y + (int)round(size * 0.52);
    SDL_Rect shoulders = { x + (size - barW)/2, barY, barW, barH };
    int barR = barH/2;
    drawRoundedRectFilled(shoulders, barR, col);
    SDL_Rect inner = { shoulders.x + stroke, shoulders.y + stroke,
                       shoulders.w - 2*stroke, shoulders.h - 2*stroke };
    drawRoundedRectFilled(inner, barR - stroke, bg);

    int neckW = (int)round(size * 0.34);
    int neckH = stroke + 2;
    SDL_Rect neck = { cx - neckW/2, cy + headR - neckH/2, neckW, neckH };
    drawRoundedRectFilled(neck, neckH/2, bg);
}

static void drawTwoPeopleOutline(int x, int y, int size, int stroke,
                                 SDL_Color col, SDL_Color bg)
{
    int bSize = (int)round(size * 0.86);
    int bx    = x + (int)round(size * 0.24);
    int by    = y - (int)round(size * 0.08);
    drawPersonOutline(bx, by, bSize, stroke, col, bg);
    drawPersonOutline(x,  y,  size,  stroke, col, bg);
}

// Button drawing
static void drawButton(SDL_Rect r, const char* label, int hovered, ButtonIcon icon)
{
    const int radius = 14;

    SDL_Color fillNormal  = {252,252,255,255};
    SDL_Color fillHover   = {236,240,245,255};
    SDL_Color borderColor = {184,192,208,255};
    SDL_Color labelColor  = {0,0,0,255};  // text always black on white button

    SDL_Rect shadow = { r.x, r.y + 2, r.w, r.h };
    SDL_Color shadowCol = {0,0,0,55};
    drawRoundedRectFilled(shadow, radius + 1, shadowCol);

    SDL_Color fill = hovered ? fillHover : fillNormal;
    drawRoundedRectFilled(r, radius, fill);
    drawRoundedRectOutline(r, radius, borderColor);

    int pad = 14;
    int iconBox = r.h - pad*2;
    SDL_Rect iconRect = { r.x + pad, r.y + pad, iconBox, iconBox };

    int stroke = iconBox / 10;
    if (stroke < 2) stroke = 2;
    SDL_Color black = {0,0,0,255};

    if (icon == ICON_SOLO)
        drawPersonOutline(iconRect.x, iconRect.y, iconRect.w, stroke, black, fill);
    else if (icon == ICON_DUO)
        drawTwoPeopleOutline(iconRect.x, iconRect.y, iconRect.w, stroke, black, fill);

    SDL_Texture* txt = createTextTexture(label, font, labelColor);
    if (!txt) return;
    int tw, th;
    SDL_QueryTexture(txt, NULL, NULL, &tw, &th);

    int textLeft = (icon==ICON_NONE) ? (r.x + pad) : (iconRect.x + iconRect.w + pad);
    int textAvail = r.x + r.w - textLeft - pad;
    if (tw > textAvail) tw = textAvail;

    SDL_Rect tdst = { textLeft, r.y + (r.h - th)/2, tw, th };
    SDL_RenderCopy(renderer, txt, NULL, &tdst);
    SDL_DestroyTexture(txt);
}

// Drawing mode buttons in main menu
static int modeMenu(void) {
    SDL_Event event;

    const int btnW = WINDOW_WIDTH - 120;
    const int btnH = 64;
    SDL_Rect soloBtn  = { (WINDOW_WIDTH - btnW)/2, WINDOW_HEIGHT/2 - btnH - 12, btnW, btnH };
    SDL_Rect duoBtn   = { (WINDOW_WIDTH - btnW)/2, WINDOW_HEIGHT/2 + 12,          btnW, btnH };
    SDL_Rect playbackBtn= { (WINDOW_WIDTH - btnW)/2, WINDOW_HEIGHT/2 + btnH + 36,   btnW, btnH };//PLAYBACK BTN
    SDL_Rect themeBtn = { WINDOW_WIDTH - 140, 20, 120, 40 }; // top-right

    for (;;) {
        setColor(getBackgroundColor());
        SDL_RenderClear(renderer);

        // title
        SDL_Texture* title = createTextTexture("Tic-Tac-Toe", font, getTextColor());
        if (title) {
            int tw, th;
            SDL_QueryTexture(title, NULL, NULL, &tw, &th);
            SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 80, tw, th };
            SDL_RenderCopy(renderer, title, NULL, &tpos);
            SDL_DestroyTexture(title);
        }

        int mx, my; SDL_GetMouseState(&mx, &my);

        //functions for hover feature
        int hSolo  = (mx>=soloBtn.x  && mx<=soloBtn.x+soloBtn.w  &&
                      my>=soloBtn.y  && my<=soloBtn.y+soloBtn.h);
        int hDuo   = (mx>=duoBtn.x   && mx<=duoBtn.x+duoBtn.w   &&
                      my>=duoBtn.y   && my<=duoBtn.y+duoBtn.h);
        int hTheme = (mx>=themeBtn.x && mx<=themeBtn.x+themeBtn.w &&
                      my>=themeBtn.y && my<=themeBtn.y+themeBtn.h);
        int hPlayback = (mx>=playbackBtn.x && mx<=playbackBtn.x+playbackBtn.w &&
                 my>=playbackBtn.y && my<=playbackBtn.y+playbackBtn.h);


        drawButton(soloBtn,  "Play Solo",          hSolo,  ICON_SOLO);
        drawButton(duoBtn,   "Play with a friend", hDuo,   ICON_DUO);
        drawButton(playbackBtn, "Playback", hPlayback, ICON_NONE);
        drawButton(themeBtn, "Theme",              hTheme, ICON_NONE);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return 0;
            //MAPS THE X AND Y COORDINATES TO THE BUTTON
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x>=themeBtn.x && x<=themeBtn.x+themeBtn.w &&
                    y>=themeBtn.y && y<=themeBtn.y+themeBtn.h) {
                    Theme chosen = themeMenu();
                    if (chosen == THEME_DARK || chosen == THEME_FUN)
                        currentTheme = chosen;
                }
                //SINGLEPLAYER MODE CLICK
                if (x>=soloBtn.x && x<=soloBtn.x+soloBtn.w &&
                    y>=soloBtn.y && y<=soloBtn.y+soloBtn.h)  return MODE_SP;
                //MULTIPLAYER MODE CLICK
                if (x>=duoBtn.x  && x<=duoBtn.x+duoBtn.w  &&
                    y>=duoBtn.y  && y<=duoBtn.y+duoBtn.h)   return MODE_MP;
                
                    // PLAYBACK CLICK
                if (x>=playbackBtn.x && x<=playbackBtn.x+playbackBtn.w && y>=playbackBtn.y && y<=playbackBtn.y+playbackBtn.h) {
                    // IF PLAYBACK DATA EXIST
                if (playback_has_last_game()) {
                    playbackScreen();
                } 
                
                else { //NO RECORDED DATA LAST GAME
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Playback", "No completed game to playback yet.", window);
                }
            }           

            }
        }
        SDL_Delay(16);
    }
}

// background theme selection
static Theme themeMenu(void) {
    SDL_Event event;

    const int btnW = WINDOW_WIDTH - 120;
    const int btnH = 64;
    const int centerX = (WINDOW_WIDTH - btnW)/2;
    const int startY  = WINDOW_HEIGHT/2 - btnH - 12;

    SDL_Rect darkBtn = { centerX, startY,             btnW, btnH };
    SDL_Rect funBtn  = { centerX, startY + btnH + 24, btnW, btnH };

    for (;;) {
        // Use current theme's background / text
        setColor(getBackgroundColor());
        SDL_RenderClear(renderer);

        SDL_Texture* title = createTextTexture("Select Theme", font, getTextColor());
        if (title) {
            int tw, th;
            SDL_QueryTexture(title, NULL, NULL, &tw, &th);
            SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 80, tw, th };
            SDL_RenderCopy(renderer, title, NULL, &tpos);
            SDL_DestroyTexture(title);
        }

        int mx, my; SDL_GetMouseState(&mx, &my);
        int hDark = (mx>=darkBtn.x && mx<=darkBtn.x+darkBtn.w &&
                     my>=darkBtn.y && my<=darkBtn.y+darkBtn.h);
        int hFun  = (mx>=funBtn.x  && mx<=funBtn.x +funBtn.w  &&
                     my>=funBtn.y  && my<=funBtn.y +funBtn.h);

        drawButton(darkBtn, "Dark Theme", hDark, ICON_NONE);
        drawButton(funBtn,  "Fun Theme",  hFun,  ICON_NONE);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return currentTheme;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x>=darkBtn.x && x<=darkBtn.x+darkBtn.w &&
                    y>=darkBtn.y && y<=darkBtn.y+darkBtn.h) {
                    return THEME_DARK;
                }
                if (x>=funBtn.x && x<=funBtn.x+funBtn.w &&
                    y>=funBtn.y && y<=funBtn.y+funBtn.h) {
                    return THEME_FUN;
                }
            }
        }
        SDL_Delay(16);
    }
}

// difficulty selection
static Difficulty difficultyMenu(void) {
    SDL_Event event;

    const int btnW = WINDOW_WIDTH - 120;
    const int btnH = 64;
    const int centerX = (WINDOW_WIDTH - btnW)/2;
    const int startY  = WINDOW_HEIGHT/2 - btnH - 12;

    SDL_Rect easyBtn   = { centerX, startY,               btnW, btnH };
    SDL_Rect medBtn    = { centerX, startY + btnH + 24,   btnW, btnH };
    SDL_Rect hardBtn   = { centerX, startY + 2*(btnH+24), btnW, btnH };

    const int backW = 120, backH = 46, pad = 24;
    SDL_Rect backBtn = { WINDOW_WIDTH - backW - pad, WINDOW_HEIGHT - backH - pad,
                         backW, backH };

    for (;;) {
        setColor(getBackgroundColor());
        SDL_RenderClear(renderer);

        SDL_Texture* title = createTextTexture("Select Difficulty", font, getTextColor());
        if (title) {
            int tw, th;
            SDL_QueryTexture(title, NULL, NULL, &tw, &th);
            SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 80, tw, th };
            SDL_RenderCopy(renderer, title, NULL, &tpos);
            SDL_DestroyTexture(title);
        }

        int mx, my; SDL_GetMouseState(&mx, &my);
        int hEasy = (mx>=easyBtn.x && mx<=easyBtn.x+easyBtn.w &&
                     my>=easyBtn.y && my<=easyBtn.y+easyBtn.h);
        int hMed  = (mx>=medBtn.x  && mx<=medBtn.x +medBtn.w  &&
                     my>=medBtn.y  && my<=medBtn.y +medBtn.h);
        int hHard = (mx>=hardBtn.x && mx<=hardBtn.x+hardBtn.w &&
                     my>=hardBtn.y && my<=hardBtn.y+hardBtn.h);
        int hBack = (mx>=backBtn.x && mx<=backBtn.x+backBtn.w &&
                     my>=backBtn.y && my<=backBtn.y+backBtn.h);

        drawButton(easyBtn, "Easy (Naive Bayes)",      hEasy, ICON_SOLO);
        drawButton(medBtn,  "Medium (Minimax)",        hMed,  ICON_SOLO);
        drawButton(hardBtn, "Hard (Perfect Minimax)",  hHard, ICON_SOLO);
        drawButton(backBtn, "Back",                    hBack, ICON_NONE);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_QUIT;
                exit(0);
            }

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x>=easyBtn.x && x<=easyBtn.x+easyBtn.w &&
                    y>=easyBtn.y && y<=easyBtn.y+easyBtn.h)  return DIFF_EASY;
                if (x>=medBtn.x  && x<=medBtn.x +medBtn.w  &&
                    y>=medBtn.y  && y<=medBtn.y +medBtn.h)  return DIFF_MEDIUM;
                if (x>=hardBtn.x && x<=hardBtn.x+hardBtn.w &&
                    y>=hardBtn.y && y<=hardBtn.y+hardBtn.h) return DIFF_HARD;
                if (x>=backBtn.x && x<=backBtn.x+backBtn.w &&
                    y>=backBtn.y && y<=backBtn.y+backBtn.h) return DIFF_BACK;
            }
        }
        SDL_Delay(16);
    }
}

// player icon selection
static PlayerSide sideMenu(void) {
    SDL_Event event;

    for(;;){
        setColor(getBackgroundColor());
        SDL_RenderClear(renderer);

        SDL_Texture *t0 = createTextTexture("Choose Your Side", font, getTextColor());
        SDL_Texture *t1 = createTextTexture("Play as X",        font, getTextColor());
        SDL_Texture *t2 = createTextTexture("Play as O",        font, getTextColor());

        int w0,h0,w1,h1,w2,h2;
        if (t0) SDL_QueryTexture(t0,NULL,NULL,&w0,&h0);
        else { w0=h0=0; }
        if (t1) SDL_QueryTexture(t1,NULL,NULL,&w1,&h1);
        else { w1=h1=0; }
        if (t2) SDL_QueryTexture(t2,NULL,NULL,&w2,&h2);
        else { w2=h2=0; }

        SDL_Rect r0={WINDOW_WIDTH/2-w0/2, 80,  w0,h0};
        SDL_Rect r1={WINDOW_WIDTH/2-w1/2, 160, w1,h1};
        SDL_Rect r2={WINDOW_WIDTH/2-w2/2, 220, w2,h2};

        if (t0) SDL_RenderCopy(renderer,t0,NULL,&r0);
        if (t1) SDL_RenderCopy(renderer,t1,NULL,&r1);
        if (t2) SDL_RenderCopy(renderer,t2,NULL,&r2);

        SDL_RenderPresent(renderer);

        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) {
                SDL_Quit();
                exit(0);


            }
            if(event.type==SDL_MOUSEBUTTONDOWN){
                int mx=event.button.x,my=event.button.y;
                if(mx>=r1.x&&mx<=r1.x+r1.w&&my>=r1.y&&my<=r1.y+r1.h){
                    if (t0) SDL_DestroyTexture(t0);
                    if (t1) SDL_DestroyTexture(t1);
                    if (t2) SDL_DestroyTexture(t2);
                    return SIDE_X;
                }
                if(mx>=r2.x&&mx<=r2.x+r2.w&&my>=r2.y&&my<=r2.y+r2.h){
                    if (t0) SDL_DestroyTexture(t0);
                    if (t1) SDL_DestroyTexture(t1);
                    if (t2) SDL_DestroyTexture(t2);
                    return SIDE_O;
                }
            }
        }
        if (t0) SDL_DestroyTexture(t0);
        if (t1) SDL_DestroyTexture(t1);
        if (t2) SDL_DestroyTexture(t2);
        SDL_Delay(16);
    }
}

// updates score of current game
static void updateScores(int winner) {
    if (winner==X) scoreX++;
    else if (winner==O) scoreO++;
}

// detects win-logic to return winner
static int checkWin(void) {
    for (int i=0;i<3;i++) {
        if (board[i][0]!=EMPTY && board[i][0]==board[i][1] && board[i][1]==board[i][2])
            return board[i][0];
        if (board[0][i]!=EMPTY && board[0][i]==board[1][i] && board[1][i]==board[2][i])
            return board[0][i];
    }
    if (board[0][0]!=EMPTY && board[0][0]==board[1][1] && board[1][1]==board[2][2])
        return board[0][0];
    if (board[0][2]!=EMPTY && board[0][2]==board[1][1] && board[1][1]==board[2][0])
        return board[0][2];
    return 0;
}

// checks if board is full
static int isBoardFull(void) {
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            if(board[i][j]==EMPTY) return 0;
    return 1;
}

// renders display message
static void displayMessage(const char* message) {
    SDL_Texture* msgTex = createTextTexture(message, font, getTextColor());
    if (!msgTex) return;

    int w, h;
    SDL_QueryTexture(msgTex, NULL, NULL, &w, &h);
    SDL_Rect dest = { (WINDOW_WIDTH  - w) / 2,
                      (WINDOW_HEIGHT - h) / 2,
                      w, h };

    SDL_RenderCopy(renderer, msgTex, NULL, &dest);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(msgTex);
    SDL_Delay(1100);
}

// AI behaviour based on difficulty
static void botMove(void) {
    int move = -1; // no move chosen yet
    if (aiDiff == DIFF_EASY) {
        move = bestMove_naive_bayes_for(board, aiPiece); // bot chooses next move based on NB training data
    } else if (aiDiff == DIFF_MEDIUM) {
        // bot chooses next move with a depth of only 3, with 20% chance to choose non-optimal move
        move = bestMove_minimax_for(board, aiPiece, 3, 20);
    } else {
        // bot chooses next move with exhaustive depth, with 0% chance to choose non-optimal move
        move = bestMove_minimax_for(board, aiPiece, -1, 0);
    }

    if (move != -1) {
        int i = move / 3;
        int j = move % 3;
        if (board[i][j] == EMPTY) {
            board[i][j] = aiPiece;
            playback_record_move(i, j, aiPiece); //record ai move
            needsRedraw = 1;
        }
    }
}

// in-game rendering
static void renderGame(void) {
    setColor(getBackgroundColor());
    SDL_RenderClear(renderer);

    int y = PADDING_TOP;

    // 1) Mode text
    const char* modeText = "Multiplayer Mode";
    char buf[64];
    if (gameMode == MODE_SP) {
        const char* m = (aiDiff==DIFF_EASY)  ? "Easy Mode" :
                        (aiDiff==DIFF_MEDIUM)? "Medium Mode" :
                                               "Hard Mode";
        snprintf(buf, sizeof(buf), "%s", m);
        modeText = buf;
    }
    SDL_Texture* modeTex = createTextTexture(modeText, font, getTextColor());
    if (modeTex) {
        int mw, mh;
        SDL_QueryTexture(modeTex, NULL, NULL, &mw, &mh);
        SDL_Rect modePos = { (WINDOW_WIDTH-mw)/2, y, mw, mh };
        SDL_RenderCopy(renderer, modeTex, NULL, &modePos);
        SDL_DestroyTexture(modeTex);

        SDL_Texture* backTex = createTextTexture("Back", font, getTextColor());
        if (backTex) {
            int bw,bh;
            SDL_QueryTexture(backTex,NULL,NULL,&bw,&bh);
            backButton = (SDL_Rect){ WINDOW_WIDTH - bw - 16, y, bw, bh };
            SDL_RenderCopy(renderer, backTex, NULL, &backButton);
            SDL_DestroyTexture(backTex);
        }
        y += mh + MODE_BOTTOM_PAD;
    }

    // 2) Score boxes
    int cardW = 170, cardH = 54, gap = 14;
    SDL_Rect leftCard  = { (WINDOW_WIDTH/2) - cardW - gap/2, y, cardW, cardH };
    SDL_Rect rightCard = { (WINDOW_WIDTH/2) + gap/2,         y, cardW, cardH };

    drawRoundedRectFilled(leftCard, 12, cardFill);
    drawRoundedRectOutline(leftCard, 12, cardBorder);
    drawRoundedRectFilled(rightCard, 12, cardFill);
    drawRoundedRectOutline(rightCard, 12, cardBorder);

    int yourWins = scoreX, cpuWins = scoreO;
    if (gameMode==MODE_SP) {
        yourWins = (playerSide==SIDE_X)? scoreX : scoreO;
        cpuWins  = (playerSide==SIDE_X)? scoreO : scoreX;
    }

    char leftText[32], rightText[32];
    if (gameMode==MODE_SP) {
        snprintf(leftText,  sizeof(leftText),  "You:  %d", yourWins);
        snprintf(rightText, sizeof(rightText), "Bot:  %d", cpuWins);
    } else {
        snprintf(leftText,  sizeof(leftText),  "X:    %d", scoreX);
        snprintf(rightText, sizeof(rightText), "O:    %d", scoreO);
    }

    // In fun theme score text should be white
    SDL_Color scoreCol = (currentTheme == THEME_FUN ? textLight : getTextColor());

    SDL_Texture* lt = createTextTexture(leftText, font, scoreCol);
    SDL_Texture* rt = createTextTexture(rightText, font, scoreCol);
    if (lt && rt) {
        int ltw,lth, rtw,rth;
        SDL_QueryTexture(lt,NULL,NULL,&ltw,&lth);
        SDL_QueryTexture(rt,NULL,NULL,&rtw,&rth);
        SDL_Rect lpos = { leftCard.x + (leftCard.w-ltw)/2,  leftCard.y + (leftCard.h-lth)/2,  ltw,lth };
        SDL_Rect rpos = { rightCard.x + (rightCard.w-rtw)/2,rightCard.y + (rightCard.h-rth)/2,rtw,rth };
        SDL_RenderCopy(renderer, lt, NULL, &lpos);
        SDL_RenderCopy(renderer, rt, NULL, &rpos);
    }
    if (lt) SDL_DestroyTexture(lt);
    if (rt) SDL_DestroyTexture(rt);

    y += cardH + SCOREBOXES_BOTTOM_PAD;

    // 3) Turn label
    const char* turnText = "";
    int turnPiece = (currentPlayer==1)? X : O;
    if (gameMode==MODE_SP) {
        Cell humanPiece = (playerSide==SIDE_X)? X : O;
        turnText = (turnPiece==humanPiece) ? "Your Turn" : "CPU's Turn";
    } else {
        turnText = (turnPiece==X) ? "Player X's Turn" : "Player O's Turn";
    }
    SDL_Texture* ttex = createTextTexture(turnText, font, getTextColor());
    if (ttex) {
        int tw,th;
        SDL_QueryTexture(ttex,NULL,NULL,&tw,&th);
        SDL_Rect tpos = { (WINDOW_WIDTH-tw)/2, y, tw, th };
        SDL_RenderCopy(renderer, ttex, NULL, &tpos);
        SDL_DestroyTexture(ttex);
        y += th + TURN_LABEL_BOTTOM_PAD;
    }

    // 4) Board
    int cardSide = 3*CELL_SIZE + 2*GRID_GAP + 2*BOARD_PAD;
    boardRect.w = cardSide; boardRect.h = cardSide;
    boardRect.x = (WINDOW_WIDTH - boardRect.w)/2;
    boardRect.y = y;

    SDL_Color boardFillUse   = boardFill;
    SDL_Color boardBorderUse = boardBorder;
    SDL_Color cellFillUse    = cellFill;
    SDL_Color cellBorderUse  = cellBorder;

    SDL_Color xIconColor = xColor;
    SDL_Color oIconColor = oColor;

    if (currentTheme == THEME_FUN) {
        boardFillUse = funBoardFill;
        cellFillUse  = funCellFill;
        SDL_Color black = {0,0,0,255};
        boardBorderUse = black;
        cellBorderUse  = black;
        xIconColor = (SDL_Color){ 20, 150, 60, 255 };
        oIconColor = (SDL_Color){180, 60,  90,255 };
    }

    drawRoundedRectFilled(boardRect, 16, boardFillUse);
    drawRoundedRectOutline(boardRect, 16, boardBorderUse);

    int gx = boardRect.x + BOARD_PAD;
    int gy = boardRect.y + BOARD_PAD;

    Uint32 tick = SDL_GetTicks();
    int blinkOn = ((tick / 400) % 2) == 0;   // ~2.5 blinks per second

    for (int r=0; r<3; ++r) {
        for (int c=0; c<3; ++c) {
            SDL_Rect cell = { gx + c*(CELL_SIZE + GRID_GAP),
                              gy + r*(CELL_SIZE + GRID_GAP),
                              CELL_SIZE, CELL_SIZE };
            int idx = r*3 + c;
            SDL_Color fill = cellFillUse;

            if (idx == hintIndex && board[r][c] == EMPTY && blinkOn) {
                fill = hintFill;
            }

            drawRoundedRectFilled(cell, 12, fill);
            drawRoundedRectOutline(cell, 12, cellBorderUse);

            int inset  = 18;
            int stroke = 14;
            if (board[r][c] == X)
                drawXIcon(cell, inset, stroke, xIconColor);
            else if (board[r][c] == O)
                drawOIcon(cell, inset, stroke, oIconColor, cellFillUse);
        }
    }

    y += boardRect.h + BOARD_BOTTOM_PAD;

    // 5) Reset button (bottom)
    int btnW = WINDOW_WIDTH - 120;
    int btnH = 56;
    resetButton = (SDL_Rect){ (WINDOW_WIDTH-btnW)/2, y, btnW, btnH };
    int mx,my; SDL_GetMouseState(&mx,&my);
    int hReset = (mx>=resetButton.x && mx<=resetButton.x+resetButton.w &&
                  my>=resetButton.y && my<=resetButton.y+resetButton.h);
    drawButton(resetButton, "Reset Game", hReset, ICON_NONE);

    SDL_RenderPresent(renderer);
    needsRedraw = 0;
}

// playback screen rendering
static void playbackScreen(void) {
    if (!playback_has_last_game()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                                 "Playback",
                                 "No completed game to playback yet.",
                                 window);
        return;
    }

    int playbackIndex = 0;
    int maxMoves      = playback_get_move_count();
    Cell pbBoard[3][3];

    SDL_Event event;
    int viewing = 1;

    while (viewing) {
        // Build board state for this step
        playback_build_board_at_step(playbackIndex, pbBoard);

        // Background based on current theme
        setColor(getBackgroundColor());
        SDL_RenderClear(renderer);

        // Title
        SDL_Texture* title = createTextTexture("Playback - Last Game", font, getTextColor());
        if (title) {
            int tw, th;
            SDL_QueryTexture(title, NULL, NULL, &tw, &th);
            SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 40, tw, th };
            SDL_RenderCopy(renderer, title, NULL, &tpos);
            SDL_DestroyTexture(title);
        }

        // Step text: "Move X / N"
        char stepBuf[64];
        snprintf(stepBuf, sizeof(stepBuf), "Move %d / %d", playbackIndex, maxMoves);
        SDL_Texture* stepTex = createTextTexture(stepBuf, font, getTextColor());
        if (stepTex) {
            int sw, sh;
            SDL_QueryTexture(stepTex, NULL, NULL, &sw, &sh);
            SDL_Rect spos = { (WINDOW_WIDTH - sw)/2, 80, sw, sh };
            SDL_RenderCopy(renderer, stepTex, NULL, &spos);
            SDL_DestroyTexture(stepTex);
        }

        // Board layout (same maths as renderGame)
        int cardSide = 3*CELL_SIZE + 2*GRID_GAP + 2*BOARD_PAD;
        boardRect.w = cardSide;
        boardRect.h = cardSide;
        boardRect.x = (WINDOW_WIDTH - boardRect.w)/2;
        boardRect.y = WINDOW_HEIGHT/2 - boardRect.h/2;

        SDL_Color boardFillUse   = boardFill;
        SDL_Color boardBorderUse = boardBorder;
        SDL_Color cellFillUse    = cellFill;
        SDL_Color cellBorderUse  = cellBorder;

        SDL_Color xIconColor = xColor;
        SDL_Color oIconColor = oColor;

        if (currentTheme == THEME_FUN) {
            boardFillUse   = funBoardFill;
            cellFillUse    = funCellFill;
            SDL_Color black = (SDL_Color){0,0,0,255};
            boardBorderUse = black;
            cellBorderUse  = black;

            xIconColor = (SDL_Color){ 20, 150,  60, 255 };
            oIconColor = (SDL_Color){180,  60,  90, 255 };
        }

        drawRoundedRectFilled(boardRect, 16, boardFillUse);
        drawRoundedRectOutline(boardRect, 16, boardBorderUse);

        int gx = boardRect.x + BOARD_PAD;
        int gy = boardRect.y + BOARD_PAD;

        for (int r=0; r<3; ++r) {
            for (int c=0; c<3; ++c) {
                SDL_Rect cell = {
                    gx + c*(CELL_SIZE + GRID_GAP),
                    gy + r*(CELL_SIZE + GRID_GAP),
                    CELL_SIZE, CELL_SIZE
                };

                drawRoundedRectFilled(cell, 12, cellFillUse);
                drawRoundedRectOutline(cell, 12, cellBorderUse);

                int inset = 18;
                int stroke = 14;
                if (pbBoard[r][c] == X) {
                    drawXIcon(cell, inset, stroke, xIconColor);
                } else if (pbBoard[r][c] == O) {
                    drawOIcon(cell, inset, stroke, oIconColor, cellFillUse);
                }
            }
        }

        // Bottom buttons: Prev / Next / Back
        int btnW = 140, btnH = 50;
        int gap  = 20;
        int totalW = btnW*3 + gap*2;
        int startX = (WINDOW_WIDTH - totalW)/2;
        int by = WINDOW_HEIGHT - 80;

        SDL_Rect prevBtn = { startX,              by, btnW, btnH };
        SDL_Rect nextBtn = { startX + btnW + gap, by, btnW, btnH };
        SDL_Rect backBtn = { startX + 2*(btnW+gap), by, btnW, btnH };

        int mx, my; SDL_GetMouseState(&mx, &my);
        int hPrev = (mx>=prevBtn.x && mx<=prevBtn.x+prevBtn.w &&
                     my>=prevBtn.y && my<=prevBtn.y+prevBtn.h);
        int hNext = (mx>=nextBtn.x && mx<=nextBtn.x+nextBtn.w &&
                     my>=nextBtn.y && my<=nextBtn.y+nextBtn.h);
        int hBack = (mx>=backBtn.x && mx<=backBtn.x+backBtn.w &&
                     my>=backBtn.y && my<=backBtn.y+backBtn.h);

        drawButton(prevBtn, "< Prev", hPrev, ICON_NONE);
        drawButton(nextBtn, "Next >", hNext, ICON_NONE);
        drawButton(backBtn, "Back",   hBack, ICON_NONE);

        SDL_RenderPresent(renderer);

        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_Quit();
                exit(0);
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                if (x>=prevBtn.x && x<=prevBtn.x+prevBtn.w &&
                    y>=prevBtn.y && y<=prevBtn.y+prevBtn.h) {
                    if (playbackIndex > 0) playbackIndex--;
                }
                if (x>=nextBtn.x && x<=nextBtn.x+nextBtn.w &&
                    y>=nextBtn.y && y<=nextBtn.y+nextBtn.h) {
                    if (playbackIndex < maxMoves) playbackIndex++;
                }
                if (x>=backBtn.x && x<=backBtn.x+backBtn.w &&
                    y>=backBtn.y && y<=backBtn.y+backBtn.h) {
                    viewing = 0;
                    break;
                }
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_LEFT) {
                    if (playbackIndex > 0) playbackIndex--;
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    if (playbackIndex < maxMoves) playbackIndex++;
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    viewing = 0;
                    break;
                }
            }
        }

        SDL_Delay(16);
    }
}

// ===================== MAIN =====================
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL)); // ensure AI blunder for each run

    // render SDL for hints
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // start SDL video system
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // initialize font rendering
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // create window
    window = SDL_CreateWindow("Tic Tac Toe",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    // create hardware-accelerated renderer
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED |
                                  SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_RenderSetIntegerScale(renderer, SDL_FALSE); // allows smoothber scaling of textures

    font = NULL;
    const char* fontPaths[] = { // use arial font
        "arial.ttf",
        "fonts/arial.ttf",
        "assets/arial.ttf"
    };

    for (int i = 0; i < 3 && !font; ++i) {  // loads 3 font paths
        font = TTF_OpenFont(fontPaths[i], 28);
    }
    if (!font) {
        fprintf(stderr, "Could not open font arial.ttf: %s\n", TTF_GetError());
        return 1;
    }
    
    // font options
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    TTF_SetFontKerning(font, 1);

    // train Naive Bayes AI
    nb_train_from_file("tic-tac-toe.data");

    // menu setup
    int modeSel = modeMenu(); // displays menu
    if (modeSel==0) goto cleanup;   // clears the page if "Exit" is pressed
    gameMode = (modeSel==MODE_SP)? MODE_SP: MODE_MP;    // sets gamemode based on user pref

    if (gameMode==MODE_SP) {
        for (;;) {
            Difficulty d = difficultyMenu(); // display the difficulty menu
            if (d != DIFF_BACK) { aiDiff = d; break; } // go back to menu if "Back" is clicked
            modeSel = modeMenu();
            if (modeSel==0) goto cleanup;
            gameMode = (modeSel==MODE_SP)? MODE_SP: MODE_MP;
            if (gameMode != MODE_SP) break; // exit back to menu if "singleplayer" is not clicked
        }
        if (gameMode==MODE_SP) {
            playerSide = sideMenu();    // option to choose which side to play us
            aiPiece = (playerSide==SIDE_X) ? O : X; // assign icon to player based on pref
        }
    }

    lastHumanActivityTicks = SDL_GetTicks();    // cjeck for player idle time

    int running = 1;
    initBoard();    // draw the board
    playback_begin_new_game();//NEW GAME PLAYBACK RECORD
    currentPlayer = firstPlayer;    // stores currentplayer so loser will start first

    // Main game loop
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) 
            running=0;  // shuts down program if program closes

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x, my = event.button.y;   // click behavaiour for mouse

                // Back button
                if (mx >= backButton.x && mx <= backButton.x + backButton.w &&
                    my >= backButton.y && my <= backButton.y + backButton.h) {

                    // int newMode = modeMenu();
                    // if (newMode==0) { 
                    //     running=0; 
                    //     break; 
                    // }
                    // gameMode = (newMode==MODE_SP)? MODE_SP: MODE_MP;

                    // if (gameMode==MODE_SP) {
                    //     for (;;) {
                    //         Difficulty d = difficultyMenu();
                    //         if (d != DIFF_BACK) { aiDiff = d; break; }
                    //         int tmpMode = modeMenu();
                    //         if (tmpMode==0) { running=0; break; }
                    //         gameMode = (tmpMode==MODE_SP)? MODE_SP: MODE_MP;
                    //         if (gameMode != MODE_SP) break;
                    //     }
                    //     if (!running) break;
                    //     if (gameMode==MODE_SP) {
                    //         playerSide = sideMenu();
                    //         aiPiece = (playerSide==SIDE_X) ? O : X;
                    //     }
                    // }

                    // resets game state
                    scoreX=scoreO=0;
                    initBoard();
                    playback_begin_new_game();
                    firstPlayer=1;
                    currentPlayer=firstPlayer;
                    needsRedraw=1;
                    continue;
                }

                // Reset button
                if (mx>=resetButton.x && mx<=resetButton.x+resetButton.w &&
                    my>=resetButton.y && my<=resetButton.y+resetButton.h) {
                    scoreX = scoreO = 0;
                    initBoard();
                    playback_begin_new_game();
                    firstPlayer = 1;
                    currentPlayer = firstPlayer;
                    needsRedraw = 1;
                    continue;
                }

                // Board clicks
                int gx = boardRect.x + BOARD_PAD;
                int gy = boardRect.y + BOARD_PAD;
                int stride = CELL_SIZE + GRID_GAP;

                if (mx >= gx && mx <= gx + 3*CELL_SIZE + 2*GRID_GAP &&
                    my >= gy && my <= gy + 3*CELL_SIZE + 2*GRID_GAP) {

                    int relx = mx - gx;
                    int rely = my - gy;
                    int c = relx / stride;
                    int r = rely / stride;
                    int inCellX = relx % stride;
                    int inCellY = rely % stride;

                    if (c>=0 && c<3 && r>=0 && r<3 &&
                        inCellX < CELL_SIZE && inCellY < CELL_SIZE) {
                        if (board[r][c] == EMPTY) {
                            Cell playerPiece = (gameMode==MODE_SP)
                                ? ((playerSide==SIDE_X) ? X : O)
                                : ((currentPlayer==1)?X:O);

                            board[r][c] = playerPiece;
                            playback_record_move(r, c, playerPiece);//record player move
                            currentPlayer = (currentPlayer == 1) ? 2 : 1;

                            hintIndex = -1;
                            lastHumanActivityTicks = SDL_GetTicks();
                            needsRedraw = 1;
                        }
                    }
                }
            }
        }

        // AI move
        if (gameMode==MODE_SP) {
            int whoseTurnPiece = (currentPlayer==1)? X : O; // chooses which piece to move
            // checks if board is not full before moving and no winners yet
            if (whoseTurnPiece == aiPiece &&
                !isBoardFull() && checkWin()==0) {
                botMove();
                currentPlayer = (currentPlayer == 1) ? 2 : 1;   // alternates player turn
            }
        }

        // Hint logic (prevent AI from winning)
        if (gameMode == MODE_SP) {
            Cell humanPiece = (playerSide == SIDE_X) ? X : O;
            Cell whoseTurnPiece = (currentPlayer == 1) ? X : O;

            if (whoseTurnPiece == humanPiece &&
                checkWin() == 0 &&
                !isBoardFull()) {

                Uint32 now = SDL_GetTicks();
                if (lastHumanActivityTicks != 0 &&
                    (now - lastHumanActivityTicks) > 5000) {
                    int idx = find_blocking_move_against_ai(board, aiPiece);
                    if (idx != hintIndex) {
                        hintIndex = idx;
                        needsRedraw = 1;
                    }
                }
            } else {
                hintIndex = -1;
            }
        }

        // Keep redrawing while hint is active to allow blinking
        if (hintIndex != -1)
            needsRedraw = 1;

        if (needsRedraw)
            renderGame();

        SDL_Delay(16);

        int winner = checkWin();
        if (winner || isBoardFull()) {
            playback_finalize_game(); 
            if (winner) {   // display winner message
                SDL_Color lineColor = (currentTheme == THEME_FUN)
                                      ? (SDL_Color){0,0,0,255}
                                      : getTextColor();
                animateWinLine(boardRect, lineColor);
            }

            updateScores(winner);

            if (winner == X)
                displayMessage(gameMode == MODE_SP
                               ? ((playerSide == SIDE_X) ? "You Win!" : "CPU (X) Wins!")
                               : "Player X Wins!");
            else if (winner == O)
                displayMessage(gameMode == MODE_SP
                               ? ((playerSide == SIDE_O) ? "You Win!" : "CPU (O) Wins!")
                               : "Player O Wins!");
            else
                displayMessage("Draw!");

            // ensures loser will start first in the next game
            if (winner == X) firstPlayer = 2;
            else if (winner == O) firstPlayer = 1;

            // redraws the board after winner is decided
            currentPlayer = firstPlayer;
            initBoard();
            needsRedraw = 1;
        }
    }

cleanup:    // clears and destroy all SDL states before closing the program
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

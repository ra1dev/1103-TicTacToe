// main.c — SDL2 Tic-Tac-Toe (vector X/O, refined hover buttons, taller layout)
// Changes in this version:
// 1) Buttons now have a *true hover state*: white → soft grey fill, consistent 1px border,
//    and a subtle drop shadow (no jaggy edges).
// 2) Increased window height + extra padding below Reset Game button.
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

// ------------ Window / board sizing ------------
#define WINDOW_WIDTH   600
#define WINDOW_HEIGHT  840   // taller so there is more space *below* Reset Game

// Visual grid sizing (cells + gaps inside a rounded "board card")
#define CELL_SIZE      140
#define GRID_GAP       12
#define BOARD_PAD      18

// Vertical spacing controls
#define PADDING_TOP                16
#define MODE_BOTTOM_PAD            24
#define SCOREBOXES_BOTTOM_PAD      26
#define TURN_LABEL_BOTTOM_PAD      18
#define BOARD_BOTTOM_PAD           36   // more space between board and Reset button
#define WINLINE_THICKNESS          8    //line thickness when user wins


// ------- Game types -------
typedef enum { EMPTY=0, X=1, O=2 } Cell;
typedef enum { MODE_MP=1, MODE_SP=2 } GameMode;
typedef enum { DIFF_BACK=-1, DIFF_EASY=0, DIFF_MEDIUM=1, DIFF_HARD=2 } Difficulty;
typedef enum { SIDE_X=0, SIDE_O=1 } PlayerSide;
typedef enum { ICON_NONE=0, ICON_SOLO=1, ICON_DUO=2 } ButtonIcon;

// ---- externs from your algorithm modules ----
int find_blocking_move_against_ai(Cell b[3][3], Cell aiPiece);
int bestMove_minimax(Cell b[3][3], int depthLimit, int blunderPct);
int bestMove_minimax_for(Cell b[3][3], Cell aiPiece, int depthLimit, int blunderPct);
int bestMove_naive_bayes(Cell b[3][3]);
int bestMove_naive_bayes_for(Cell b[3][3], Cell aiPiece);
void nb_train_from_file(const char* path);
// forward declaration
static void renderGame(void);


// ---- global state ----
Cell board[3][3];
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;


//------prevent AI from winning 
int hintIndex = -1;                 // -1 = no hint, 0..8 = r*3+c to highlight
Uint32 lastHumanActivityTicks = 0;  // when the human last clicked / started turn
static const SDL_Color hintFill   = { 70,  96, 140, 255 };   // subtle highlight

int currentPlayer = 1;          // 1 = Player X turn, 2 = Player O turn
GameMode gameMode = MODE_SP;    // default
Difficulty aiDiff = DIFF_EASY;  // default for SP
PlayerSide playerSide = SIDE_X; // default: human plays X
Cell aiPiece = O;               // derived from playerSide
int scoreX = 0, scoreO = 0;
int firstPlayer = 1;            // 1 -> X starts, 2 -> O starts

// UI state
int needsRedraw = 1;
SDL_Rect backButton = {0};
SDL_Rect resetButton = {0};
SDL_Rect boardRect  = {0};

// Colors
static const SDL_Color xColor     = { 60, 230,  90, 255}; // greenish X
static const SDL_Color oColor     = {250, 160, 170, 255}; // soft pink O
static const SDL_Color textLight  = {230, 235, 255, 255};
static const SDL_Color textDark   = { 20,  24,  32, 255};
static const SDL_Color cardFill   = { 28,  36,  56, 255};
static const SDL_Color cardBorder = { 70,  80, 110, 255};
static const SDL_Color boardFill  = { 30,  42,  66, 255};
static const SDL_Color boardBorder= { 60,  78, 110, 255};
static const SDL_Color cellFill   = { 40,  54,  82, 255};
static const SDL_Color cellBorder = { 70,  86, 120, 255};

// ---------- basics ----------
static void initBoard(void) {
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) board[i][j] = EMPTY;
}

static SDL_Texture* createTextTexture(const char* text, TTF_Font* fnt, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(fnt, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}


// ---------- Winning Line Animation Helpers ----------

// Draw a thick line between two points (used for win animation)
static void drawThickLine(float x1, float y1, float x2, float y2, int thickness, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len == 0) return;
    dx /= len; dy /= len;
    float nx = -dy, ny = dx;
    int half = thickness / 2;
    for (int i = -half; i <= half; i++) {
        float ox = nx * i, oy = ny * i;
        SDL_RenderDrawLine(renderer, (int)(x1 + ox), (int)(y1 + oy), (int)(x2 + ox), (int)(y2 + oy));
    }
}

// Find the winning line coordinates
static int getWinLine(int *r1, int *c1, int *r2, int *c2) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != EMPTY && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            *r1 = i; *c1 = 0; *r2 = i; *c2 = 2; return board[i][0];
        }
        if (board[0][i] != EMPTY && board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
            *r1 = 0; *c1 = i; *r2 = 2; *c2 = i; return board[0][i];
        }
    }
    if (board[0][0] != EMPTY && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        *r1 = 0; *c1 = 0; *r2 = 2; *c2 = 2; return board[0][0];
    }
    if (board[0][2] != EMPTY && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        *r1 = 0; *c1 = 2; *r2 = 2; *c2 = 0; return board[0][2];
    }
    return 0;
}

// Animate the winning line (0.6s linear animation)
// Smooth animated winning line (0.6s), solid opaque white, with subtle overshoot.
// Requires: getWinLine(int*,int*,int*,int*) and renderGame(void)
// Uses: CELL_SIZE, GRID_GAP, BOARD_PAD, WINLINE_THICKNESS, renderer
static void animateWinLine(SDL_Rect boardRect, SDL_Color lineColor /* pass {255,255,255,255} */) {
    int r1, c1, r2, c2;
    int winner = getWinLine(&r1, &c1, &r2, &c2);
    if (!winner) return;

    const int cellStep = CELL_SIZE + GRID_GAP;
    const int bx = boardRect.x + BOARD_PAD;
    const int by = boardRect.y + BOARD_PAD;

    // Centers of the two winning cells
    float x1 = bx + c1 * cellStep + CELL_SIZE * 0.5f;
    float y1 = by + r1 * cellStep + CELL_SIZE * 0.5f;
    float x2 = bx + c2 * cellStep + CELL_SIZE * 0.5f;
    float y2 = by + r2 * cellStep + CELL_SIZE * 0.5f;

    // Extend the line a bit past the cells (overshoot for a polished look)
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len > 0.0f) {
        float extend = 0.12f * len;   // 12% overshoot
        dx /= len; dy /= len;
        x1 -= dx * extend;  y1 -= dy * extend;
        x2 += dx * extend;  y2 += dy * extend;
    }

    // Fully opaque line (no blending)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const float duration_ms = 600.0f; // 0.6s
    Uint32 start = SDL_GetTicks();
    for (;;) {
        float t = (SDL_GetTicks() - start) / duration_ms;
        if (t > 1.0f) t = 1.0f;

        // Redraw the game behind the line each frame
        renderGame();

        // Current end point along the line
        float cx = x1 + (x2 - x1) * t;
        float cy = y1 + (y2 - y1) * t;

        // Draw the animated segment
        SDL_SetRenderDrawColor(renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
        // thick line by offsetting perpendicular
        {
            float ldx = x2 - x1, ldy = y2 - y1;
            float llen = sqrtf(ldx*ldx + ldy*ldy);
            float nx = 0.0f, ny = 0.0f;
            if (llen > 0.0f) { nx = -ldy/llen; ny = ldx/llen; }
            int half = WINLINE_THICKNESS / 2;
            for (int i = -half; i <= half; ++i) {
                int ox = (int)roundf(nx * i);
                int oy = (int)roundf(ny * i);
                SDL_RenderDrawLine(renderer, (int)(x1 + ox), (int)(y1 + oy), (int)(cx + ox), (int)(cy + oy));
            }
        }

        SDL_RenderPresent(renderer);
        if (t >= 1.0f) break;
        SDL_Delay(16); // ~60 FPS
    }

    // Ensure final full-length line is drawn once more (crisp finish)
    renderGame();
    {
        float ldx = x2 - x1, ldy = y2 - y1;
        float llen = sqrtf(ldx*ldx + ldy*ldy);
        float nx = 0.0f, ny = 0.0f;
        if (llen > 0.0f) { nx = -ldy/llen; ny = ldx/llen; }
        int half = WINLINE_THICKNESS / 2;
        for (int i = -half; i <= half; ++i) {
            int ox = (int)roundf(nx * i);
            int oy = (int)roundf(ny * i);
            SDL_RenderDrawLine(renderer, (int)(x1 + ox), (int)(y1 + oy), (int)(x2 + ox), (int)(y2 + oy));
        }
    }
    SDL_RenderPresent(renderer);
}






// -------------- rounded rect + vector primitives --------------
static void setColor(SDL_Color c){ SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a); }

static void hline(int x1, int x2, int y) {
    if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
    SDL_RenderDrawLine(renderer, x1, y, x2, y);
}

static void fillCircle(int cx, int cy, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        int dx = (int)(sqrt((double)r*r - (double)dy*dy) + 0.5);
        hline(cx - dx, cx + dx, cy + dy);
    }
}

// Draw a ring with thickness by painting outer then "punching" inner with bg
static void fillRing(int cx, int cy, int outerR, int thickness, SDL_Color ringColor, SDL_Color bgColor) {
    setColor(ringColor);
    fillCircle(cx, cy, outerR);
    int innerR = outerR - thickness;
    if (innerR > 0) {
        setColor(bgColor);
        fillCircle(cx, cy, innerR);
    }
}

static void drawRoundedRectFilled(SDL_Rect r, int rad, SDL_Color fill) {
    if (rad < 1) { setColor(fill); SDL_RenderFillRect(renderer, &r); return; }
    if (rad*2 > r.w) rad = r.w/2;
    if (rad*2 > r.h) rad = r.h/2;

    setColor(fill);

    // center body
    SDL_Rect mid = { r.x + rad, r.y, r.w - 2*rad, r.h };
    SDL_RenderFillRect(renderer, &mid);

    // left/right rectangles
    SDL_Rect left = { r.x, r.y + rad, rad, r.h - 2*rad };
    SDL_Rect right= { r.x + r.w - rad, r.y + rad, rad, r.h - 2*rad };
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);

    // corners via filled quarter-circles
    for (int dy = -rad; dy <= rad; ++dy) {
        int dx = (int)(sqrt((double)rad*rad - (double)dy*dy) + 0.5);
        // TL
        hline(r.x + rad - dx, r.x + rad, r.y + rad - dy);
        // TR
        hline(r.x + r.w - rad, r.x + r.w - rad + dx, r.y + rad - dy);
        // BL
        hline(r.x + rad - dx, r.x + rad, r.y + r.h - rad + dy);
        // BR
        hline(r.x + r.w - rad, r.x + r.w - rad + dx, r.y + r.h - rad + dy);
    }
}

static void drawRoundedRectOutline(SDL_Rect r, int rad, SDL_Color border) {
    if (rad < 1) { setColor(border); SDL_RenderDrawRect(renderer, &r); return; }
    if (rad*2 > r.w) rad = r.w/2;
    if (rad*2 > r.h) rad = r.h/2;

    setColor(border);

    // straight edges
    SDL_RenderDrawLine(renderer, r.x + rad, r.y, r.x + r.w - rad - 1, r.y);
    SDL_RenderDrawLine(renderer, r.x + rad, r.y + r.h - 1, r.x + r.w - rad - 1, r.y + r.h - 1);
    SDL_RenderDrawLine(renderer, r.x, r.y + rad, r.x, r.y + r.h - rad - 1);
    SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y + rad, r.x + r.w - 1, r.y + r.h - rad - 1);

    // corners
    for (int a = 0; a <= rad; ++a) {
        int b = (int)(sqrt((double)rad*rad - (double)a*a) + 0.5);
        // TL
        SDL_RenderDrawPoint(renderer, r.x + rad - a, r.y + rad - b);
        SDL_RenderDrawPoint(renderer, r.x + rad - b, r.y + rad - a);
        // TR
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + a, r.y + rad - b);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + b, r.y + rad - a);
        // BL
        SDL_RenderDrawPoint(renderer, r.x + rad - a, r.y + r.h - rad - 1 + b);
        SDL_RenderDrawPoint(renderer, r.x + rad - b, r.y + r.h - rad - 1 + a);
        // BR
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + a, r.y + r.h - rad - 1 + b);
        SDL_RenderDrawPoint(renderer, r.x + r.w - rad - 1 + b, r.y + r.h - rad - 1 + a);
    }
}

// --- Vector icons ---
// X: two thick diagonals drawn by offsetting lines around the centerline
static void drawThickDiag(int x1, int y1, int x2, int y2, int thickness, SDL_Color color) {
    setColor(color);
    double dx = (double)(x2 - x1), dy = (double)(y2 - y1);
    double len = sqrt(dx*dx + dy*dy); if (len == 0) return;
    double nx = -dy/len, ny = dx/len; // unit normal
    int half = thickness/2;
    for (int t = -half; t <= half; ++t) {
        int ox = (int)round(nx * t);
        int oy = (int)round(ny * t);
        SDL_RenderDrawLine(renderer, x1+ox, y1+oy, x2+ox, y2+oy);
    }
}

static void drawXIcon(SDL_Rect cell, int inset, int thickness, SDL_Color color) {
    int x0 = cell.x + inset;
    int y0 = cell.y + inset;
    int x1 = cell.x + cell.w - inset;
    int y1 = cell.y + cell.h - inset;
    drawThickDiag(x0, y0, x1, y1, thickness, color);
    drawThickDiag(x0, y1, x1, y0, thickness, color);
}

// O: draw a ring (outer – inner) with specified thickness
static void drawOIcon(SDL_Rect cell, int inset, int thickness, SDL_Color color, SDL_Color bgColor) {
    int cx = cell.x + cell.w/2;
    int cy = cell.y + cell.h/2;
    int outerR = (cell.w < cell.h ? cell.w : cell.h)/2 - inset;
    if (outerR < thickness) outerR = thickness;
    fillRing(cx, cy, outerR, thickness, color, bgColor);
}

// ----- Simple person icons used in menu buttons -----

// HELPS TO DRAW ROUNDED RECTANGLE
static void drawRoundedBarStroke(SDL_Rect r, int radius, int stroke, SDL_Color color, SDL_Color bg) {
    if (radius*2 > r.w) radius = r.w/2;
    if (radius*2 > r.h) radius = r.h/2;
    if (stroke < 1) stroke = 1;

    drawRoundedRectFilled(r, radius, color);
    SDL_Rect inner = { r.x + stroke, r.y + stroke, r.w - 2*stroke, r.h - 2*stroke };
    int innerRad = radius - stroke;
    if (inner.w > 0 && inner.h > 0 && innerRad >= 0) {
        drawRoundedRectFilled(inner, innerRad, bg);
    }
}

// HELP TO DRAW THE CIRCLE
static void drawHeadRing(int cx, int cy, int outerR, int stroke, SDL_Color color, SDL_Color bg) {
    if (outerR < stroke) outerR = stroke;
    fillRing(cx, cy, outerR, stroke, color, bg);
}




// NEW (icons in solid black)
// Professional single-person glyph: ring head + stroked shoulders


// Outline single-person (Material/Fluent-style)
static void drawPersonOutline(int x, int y, int size, int stroke, SDL_Color col, SDL_Color bg) {
    // scale
    if (stroke < 1) stroke = 1;
    int headR   = (int)round(size * 0.28);        // head radius
    int cx      = x + size/2;
    int cy      = y + (int)round(size * 0.28);    // head center Y
    // head ring
    fillRing(cx, cy, headR, stroke, col, bg);

    // shoulders/torso as rounded bar stroke (open on top)
    int barW = (int)round(size * 0.88);
    int barH = (int)round(size * 0.40);
    int barY = y + (int)round(size * 0.52);
    SDL_Rect shoulders = { x + (size - barW)/2, barY, barW, barH };
    int barR = barH/2;
    drawRoundedBarStroke(shoulders, barR, stroke, col, bg);

    // carve a small "neck gap" so ring and bar don't merge
    int neckW = (int)round(size * 0.34);
    int neckH = (int)round(stroke + 2);
    SDL_Rect neck = { cx - neckW/2, cy + headR - neckH/2, neckW, neckH };
    drawRoundedRectFilled(neck, neckH/2, bg);
}

// Outline two-people (rear + front) with slight offset, both stroked
static void drawTwoPeopleOutline(int x, int y, int size, int stroke, SDL_Color col, SDL_Color bg) {
    // back person (slightly smaller + offset up-right)
    int bSize = (int)round(size * 0.86);
    int bx = x + (int)round(size * 0.24);
    int by = y - (int)round(size * 0.08);
    drawPersonOutline(bx, by, bSize, stroke, col, bg);

    // front person
    drawPersonOutline(x, y, size, stroke, col, bg);
}

// -------------- Rounded button with refined hover (no jaggy edges) --------------
static void drawButton(SDL_Rect r, const char* label, int hovered, ButtonIcon icon) {
    const int radius = 14;
    const SDL_Color fillNormal  = {252,252,255,255};
    const SDL_Color fillHover   = {236,240,245,255};
    const SDL_Color borderColor = {184,192,208,255};
    const SDL_Color labelColor  = { 20, 24, 32,255};

    // shadow + fill + border ... (unchanged)
    SDL_Rect shadow = { r.x, r.y + 2, r.w, r.h };
    SDL_Color shadowCol = {0,0,0,55};
    drawRoundedRectFilled(shadow, radius + 1, shadowCol);

    SDL_Color fill = hovered ? fillHover : fillNormal;
    drawRoundedRectFilled(r, radius, fill);
    drawRoundedRectOutline(r, radius, borderColor);

    // --- ICON (updated) ---
    int pad = 14;
    int iconBox = r.h - pad*2;
    SDL_Rect iconRect = { r.x + pad, r.y + pad, iconBox, iconBox };
    int stroke = iconRect.w / 10; if (stroke < 2) stroke = 2;
    SDL_Color black = (SDL_Color){0,0,0,255};

    if (icon == ICON_SOLO) {
        drawPersonOutline(iconRect.x, iconRect.y, iconRect.w, stroke, black, fill);
    } else if (icon == ICON_DUO) {
        drawTwoPeopleOutline(iconRect.x, iconRect.y, iconRect.w, stroke, black, fill);
    }

    // label ... (unchanged)
    SDL_Texture* txt = createTextTexture(label, font, labelColor);
    int tw, th; SDL_QueryTexture(txt, NULL, NULL, &tw, &th);
    int textLeft = (icon==ICON_NONE) ? (r.x + pad) : (iconRect.x + iconRect.w + pad);
    int textAvail = r.x + r.w - textLeft - pad;
    if (tw > textAvail) tw = textAvail;
    SDL_Rect tdst = { textLeft, r.y + (r.h - th)/2, tw, th };
    SDL_RenderCopy(renderer, txt, NULL, &tdst);
    SDL_DestroyTexture(txt);
}


// ---------- RENDERS MAIN MENU SCREEN RENDERS AND RETURN IF USER SELECT SINGLE PLAYER OR MULTIPLAYER----------
static int modeMenu(void) {
    SDL_Event event;
    
    //DEFINE BUTTON SIZE
    const int btnW = WINDOW_WIDTH - 120;// ENSURE THAT THERE ARE SIDE MARGINES
    const int btnH = 64;//SETS THE HEIGHT OF EACH BUTTON
    SDL_Rect soloBtn  = { (WINDOW_WIDTH - btnW)/2, WINDOW_HEIGHT/2 - btnH - 12, btnW, btnH }; //sets the SINGLEPLAYER BUTTON
    SDL_Rect duoBtn   = { (WINDOW_WIDTH - btnW)/2, WINDOW_HEIGHT/2 + 12,          btnW, btnH };//Sets the MULTIPLAYER BUTTON
    for (;;) { //INFINITE LOOP, READ MOUSE INPUT AND WAIT FOR USE TO SELECT CHOICE
        //DRAW BACKGROUND, TITLE, CHECK MOUSE POSITION FOR HOVER, RENDER BUTTONS, PROCESS EVENTS DELAY FOR OPTIMIZATION

        // black background
        setColor((SDL_Color){0,0,0,255});
        SDL_RenderClear(renderer);

        // RENDER TIC TAC TOE TITLE
        SDL_Texture* title = createTextTexture("Tic-Tac-Toe", font, textLight);
        int tw, th; SDL_QueryTexture(title, NULL, NULL, &tw, &th);
        SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 80, tw, th };
        SDL_RenderCopy(renderer, title, NULL, &tpos);
        SDL_DestroyTexture(title);

        // HOVER FUNCTION, IF MOUSE COORDINATES ARE IN THE RANGE OF THE BUTTONS SET IT TO HOVER
        int mx, my; SDL_GetMouseState(&mx, &my);
        int hSolo  = (mx>=soloBtn.x  && mx<=soloBtn.x+soloBtn.w  && my>=soloBtn.y  && my<=soloBtn.y+soloBtn.h);
        int hDuo   = (mx>=duoBtn.x   && mx<=duoBtn.x+duoBtn.w   && my>=duoBtn.y   && my<=duoBtn.y+duoBtn.h);

        //INITIALIZE THE BUTTONS
        drawButton(soloBtn,  "Play Solo",          hSolo,  ICON_SOLO);
        drawButton(duoBtn,   "Play with a friend", hDuo,   ICON_DUO);

        //RENDER EVERYTHING TO THE SCREEN
        SDL_RenderPresent(renderer);


        //EVENT HANDLER, WHAT HAPPENDS WHEN AN ACTION IS PERFORMED, for example when a user click SP_MODE
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return 0;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;//get mouse coordinates check if the coordinates are in the button
                if (x>=soloBtn.x && x<=soloBtn.x+soloBtn.w && y>=soloBtn.y && y<=soloBtn.y+soloBtn.h)  return MODE_SP;
                if (x>=duoBtn.x  && x<=duoBtn.x+duoBtn.w  && y>=duoBtn.y  && y<=duoBtn.y+duoBtn.h)   return MODE_MP;
            }
        }
        SDL_Delay(16);// DELAY BY 16MS WHICH IS AROUND 60FPS, IF TOO FAST WILL BURN OUT THE CPU
    }
}



// ---------- RENDERS MAIN DIFFICULTY LVL SCREEN RENDERS AND RETURN IF USER SELECT EASY, MEDIUM HARD OR BACK BTN----------
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
    SDL_Rect backBtn = { WINDOW_WIDTH - backW - pad, WINDOW_HEIGHT - backH - pad, backW, backH };

    for (;;) {
        setColor((SDL_Color){0,0,0,255});
        SDL_RenderClear(renderer);

        SDL_Texture* title = createTextTexture("Select Difficulty", font, textLight);
        int tw, th; SDL_QueryTexture(title, NULL, NULL, &tw, &th);
        SDL_Rect tpos = { (WINDOW_WIDTH - tw)/2, 80, tw, th };
        SDL_RenderCopy(renderer, title, NULL, &tpos);
        SDL_DestroyTexture(title);

        int mx, my; SDL_GetMouseState(&mx, &my);
        int hEasy = (mx>=easyBtn.x && mx<=easyBtn.x+easyBtn.w && my>=easyBtn.y && my<=easyBtn.y+easyBtn.h);
        int hMed  = (mx>=medBtn.x  && mx<=medBtn.x +medBtn.w  && my>=medBtn.y  && my<=medBtn.y +medBtn.h);
        int hHard = (mx>=hardBtn.x && mx<=hardBtn.x+hardBtn.w && my>=hardBtn.y && my<=hardBtn.y+hardBtn.h);
        int hBack = (mx>=backBtn.x && mx<=backBtn.x+backBtn.w && my>=backBtn.y && my<=backBtn.y+backBtn.h);

        drawButton(easyBtn, "Easy (Naive Bayes)",      hEasy, ICON_SOLO);
        drawButton(medBtn,  "Medium (Minimax)",        hMed,  ICON_SOLO);
        drawButton(hardBtn, "Hard (Perfect Minimax)",  hHard, ICON_SOLO);
        drawButton(backBtn, "Back", hBack, ICON_NONE);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return DIFF_BACK;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x>=easyBtn.x && x<=easyBtn.x+easyBtn.w && y>=easyBtn.y && y<=easyBtn.y+easyBtn.h)  return DIFF_EASY;
                if (x>=medBtn.x  && x<=medBtn.x +medBtn.w  && y>=medBtn.y  && y<=medBtn.y +medBtn.h)  return DIFF_MEDIUM;
                if (x>=hardBtn.x && x<=hardBtn.x+hardBtn.w && y>=hardBtn.y && y<=hardBtn.y+hardBtn.h) return DIFF_HARD;
                if (x>=backBtn.x && x<=backBtn.x+backBtn.w && y>=backBtn.y && y<=backBtn.y+backBtn.h) return DIFF_BACK;
            }
        }
        SDL_Delay(16);
    }
}


//RENDERS IF USER WANTS TO BE X OR O, FUNCTION RETURNS SIDE_X OR SIDE_O
static PlayerSide sideMenu(void) {
    SDL_Event event;
    for(;;){
        setColor((SDL_Color){0,0,0,255});
        SDL_RenderClear(renderer);
        SDL_Texture* t0=createTextTexture("Choose Your Side",font,textLight);
        SDL_Texture* t1=createTextTexture("Play as X",font,textLight);
        SDL_Texture* t2=createTextTexture("Play as O",font,textLight);
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

// ---------- Game helpers ----------
static void updateScores(int winner) { if (winner==X) scoreX++; else if (winner==O) scoreO++; }


//function to check if anyone won or not
//if statement checks if any row or column is filled up
//returns the value X or O depending on which line is filled up
static int checkWin(void) {
    for (int i=0;i<3;i++) {
        if (board[i][0]!=EMPTY && board[i][0]==board[i][1] && board[i][1]==board[i][2]) return board[i][0];
        if (board[0][i]!=EMPTY && board[0][i]==board[1][i] && board[1][i]==board[2][i]) return board[0][i];
    }
    if (board[0][0]!=EMPTY && board[0][0]==board[1][1] && board[1][1]==board[2][2]) return board[0][0];
    if (board[0][2]!=EMPTY && board[0][2]==board[1][1] && board[1][1]==board[2][0]) return board[0][2];
    return 0;
}


//checks if the board is filled up    LOOP ROW              COLUMN             return 0 is not full return 1 if full
static int isBoardFull(void) { for(int i=0;i<3;i++) for(int j=0;j<3;j++) if(board[i][j]==EMPTY) return 0; return 1; }



// DISPLAYS YOU WIN MESSAGE
static void displayMessage(const char* message) {
    SDL_Color color = (SDL_Color){255, 255, 255, 255};

    SDL_Texture* msgTex = createTextTexture(message, font, color);
    if (!msgTex) return;  // optional safety check

    int w, h;
    SDL_QueryTexture(msgTex, NULL, NULL, &w, &h);

    SDL_Rect dest = {
        (WINDOW_WIDTH  - w) / 2,
        (WINDOW_HEIGHT - h) / 2,
        w,
        h
    };

    SDL_RenderCopy(renderer, msgTex, NULL, &dest);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(msgTex);

    SDL_Delay(1100);
}



static void botMove(void) {
    int move = -1;
    if (aiDiff == DIFF_EASY)         move = bestMove_naive_bayes_for(board, aiPiece);
    else if (aiDiff == DIFF_MEDIUM)  move = bestMove_minimax_for(board, aiPiece, 3, 20);
    else                              move = bestMove_minimax_for(board, aiPiece, -1, 0);
    if (move != -1) {
        int i = move / 3, j = move % 3;
        board[i][j] = aiPiece;
        needsRedraw = 1;
    }
}

// ---------- In-game rendering ----------
//includes, clears to black
//shows current mode, easy diff or hard
//shows the in game back button
//shows the scores
//shows whose turn
//reset game button
static void renderGame(void) {
    // Background
    setColor((SDL_Color){0,0,0,255});
    SDL_RenderClear(renderer);

    int y = PADDING_TOP;

    // 1) Top: Mode text (centered)
    const char* modeText = "Multiplayer Mode";
    char buf[64];
    if (gameMode == MODE_SP) {
        const char* m = (aiDiff==DIFF_EASY)?"Easy Mode":(aiDiff==DIFF_MEDIUM)?"Medium Mode":"Hard Mode";
        snprintf(buf, sizeof(buf), "%s", m);
        modeText = buf;
    }
    SDL_Texture* modeTex = createTextTexture(modeText, font, textLight);
    int mw, mh; SDL_QueryTexture(modeTex, NULL, NULL, &mw, &mh);
    SDL_Rect modePos = { (WINDOW_WIDTH-mw)/2, y, mw, mh };
    SDL_RenderCopy(renderer, modeTex, NULL, &modePos);
    SDL_DestroyTexture(modeTex);

    // Back (top-right)
    SDL_Texture* backTex = createTextTexture("Back", font, textLight);
    int bw,bh; SDL_QueryTexture(backTex,NULL,NULL,&bw,&bh);
    backButton = (SDL_Rect){ WINDOW_WIDTH - bw - 16, y, bw, bh };
    SDL_RenderCopy(renderer, backTex, NULL, &backButton);
    SDL_DestroyTexture(backTex);

    y += mh + MODE_BOTTOM_PAD;

    // 2) Two score boxes (left/right)
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
    SDL_Texture* lt = createTextTexture(leftText, font, textLight);
    SDL_Texture* rt = createTextTexture(rightText, font, textLight);
    int ltw,lth, rtw,rth;
    SDL_QueryTexture(lt,NULL,NULL,&ltw,&lth);
    SDL_QueryTexture(rt,NULL,NULL,&rtw,&rth);
    SDL_Rect lpos = { leftCard.x + (leftCard.w-ltw)/2,  leftCard.y + (leftCard.h-lth)/2,  ltw,lth };
    SDL_Rect rpos = { rightCard.x + (rightCard.w-rtw)/2, rightCard.y + (rightCard.h-rth)/2, rtw,rth };
    SDL_RenderCopy(renderer, lt, NULL, &lpos);
    SDL_RenderCopy(renderer, rt, NULL, &rpos);
    SDL_DestroyTexture(lt); SDL_DestroyTexture(rt);

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
    SDL_Texture* ttex = createTextTexture(turnText, font, textLight);
    int tw,th; SDL_QueryTexture(ttex,NULL,NULL,&tw,&th);
    SDL_Rect tpos = { (WINDOW_WIDTH-tw)/2, y, tw, th };
    SDL_RenderCopy(renderer, ttex, NULL, &tpos);
    SDL_DestroyTexture(ttex);

    y += th + TURN_LABEL_BOTTOM_PAD;

    // 4) Board card and grid
    int cardSide = 3*CELL_SIZE + 2*GRID_GAP + 2*BOARD_PAD;
    boardRect.w = cardSide; boardRect.h = cardSide;
    boardRect.x = (WINDOW_WIDTH - boardRect.w)/2;
    boardRect.y = y;

    drawRoundedRectFilled(boardRect, 16, boardFill);
    drawRoundedRectOutline(boardRect, 16, boardBorder);

    // grid cells and vector pieces
    int gx = boardRect.x + BOARD_PAD;
    int gy = boardRect.y + BOARD_PAD;
    for (int r=0; r<3; ++r) {
        for (int c=0; c<3; ++c) {
            SDL_Rect cell = { gx + c*(CELL_SIZE + GRID_GAP),
                            gy + r*(CELL_SIZE + GRID_GAP),
                            CELL_SIZE, CELL_SIZE };

            int idx = r*3 + c;
            SDL_Color fill = cellFill;

            // Highlight only if it's the suggested blocking move AND still empty
            if (idx == hintIndex && board[r][c] == EMPTY) {
                fill = hintFill;
            }

            drawRoundedRectFilled(cell, 12, fill);
            drawRoundedRectOutline(cell, 12, cellBorder);

            // draw vector X/O
            int inset = 18;
            int stroke = 14;
            if (board[r][c] == X) {
                drawXIcon(cell, inset, stroke, xColor);
            } else if (board[r][c] == O) {
                drawOIcon(cell, inset, stroke, oColor, cellFill);
            }
        }
    }


    y += boardRect.h + BOARD_BOTTOM_PAD;

    // 5) Reset Game button (centered under board, with hover)
    int btnW = WINDOW_WIDTH - 120;
    int btnH = 56;
    resetButton = (SDL_Rect){ (WINDOW_WIDTH-btnW)/2, y, btnW, btnH };
    int mx, my; SDL_GetMouseState(&mx,&my);
    int hReset = (mx>=resetButton.x && mx<=resetButton.x+resetButton.w &&
                  my>=resetButton.y && my<=resetButton.y+resetButton.h);
    drawButton(resetButton, "Reset Game", hReset, ICON_NONE);

    SDL_RenderPresent(renderer);
    needsRedraw = 0;
}

// ===================== MAIN LOOP =====================
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // Make the render smooth
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");//allows mouse click when im not in the windows

    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return 1; }//helps to render video
    if (TTF_Init() == -1) { fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError()); SDL_Quit(); return 1; }

    window = SDL_CreateWindow("Tic Tac Toe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);//creates the window TTT
    if (!window) { fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); return 1; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);//accelerated with VSYNC
    if (!renderer) { fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError()); return 1; }// Make it faster
    SDL_RenderSetIntegerScale(renderer, SDL_FALSE);


    // a font is used for labels; X/O are vectors now
    // Load font from local project files (no OS-specific paths)
    font = NULL;

    const char* fontPaths[] = {
        "arial.ttf",          // same folder as the .exe
        "fonts/arial.ttf",    // fonts subfolder
        "assets/arial.ttf"    // assets subfolder
    };

    for (int i = 0; i < 3 && !font; ++i) {
        font = TTF_OpenFont(fontPaths[i], 28);
    }

    if (!font) {
        fprintf(stderr, "Could not open any font arial.ttf): %s\n",
                TTF_GetError());
        return 1;
    }

    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    TTF_SetFontKerning(font, 1);
    // Train NB (file must be in same folder)
    nb_train_from_file("tic-tac-toe.data");

    // ----- Menus -----
    // LOAD THE MAIN MENU LET USER SELECT MENU
    int modeSel = modeMenu();//MODEMENU RETURNS SP OR MP MODE
    if (modeSel==0) goto cleanup;
    gameMode = (modeSel==MODE_SP)? MODE_SP: MODE_MP;

    // SINGLEPLAYER MODE
    if (gameMode==MODE_SP) {
        //INFINITE LOOP
        for (;;) {
            //LET USER SELECT EASY MEDIUM OR HARD OR BACK BTN
            Difficulty d = difficultyMenu();
            if (d != DIFF_BACK) { aiDiff = d; break; }
            modeSel = modeMenu();
            if (modeSel==0) goto cleanup;
            gameMode = (modeSel==MODE_SP)? MODE_SP: MODE_MP;
            if (gameMode != MODE_SP) break;
        }
        lastHumanActivityTicks = SDL_GetTicks();
        if (gameMode==MODE_SP) {
            playerSide = sideMenu();
            aiPiece = (playerSide==SIDE_X) ? O : X;
        }
    }

    int running = 1;
    initBoard();
    currentPlayer = firstPlayer; // 1:X, 2:O

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running=0;//IF USER QUIT GAME

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x, my = event.button.y;//CAPTURE COORDINATE OF MOUSE CLICK

                // IF USER CLICKS BACK; LOAD MAIN MENU
                if (mx >= backButton.x && mx <= backButton.x + backButton.w &&
                    my >= backButton.y && my <= backButton.y + backButton.h) {
                    // Go back to main menu flow
                    int newMode = modeMenu(); if (newMode==0) { running=0; break; }
                    gameMode = (newMode==MODE_SP)? MODE_SP: MODE_MP;
                    if (gameMode==MODE_SP) {
                        for (;;) {
                            Difficulty d = difficultyMenu();
                            if (d != DIFF_BACK) { aiDiff = d; break; }
                            int tmpMode = modeMenu();
                            if (tmpMode==0) { running=0; break; }
                            gameMode = (tmpMode==MODE_SP)? MODE_SP: MODE_MP;
                            if (gameMode != MODE_SP) break;
                        }
                        if (!running) break;
                        if (gameMode==MODE_SP) {
                            playerSide = sideMenu();
                            aiPiece = (playerSide==SIDE_X) ? O : X;
                        }
                    }
                    // reset SCORES AND RESET GAME STATE when returning to game
                    scoreX=scoreO=0; initBoard(); firstPlayer=1; currentPlayer=firstPlayer; needsRedraw=1; continue;
                }

                // RESET BUTTON, CLEAR USER SCORE, AND START OVER
                if (mx>=resetButton.x && mx<=resetButton.x+resetButton.w &&
                    my>=resetButton.y && my<=resetButton.y+resetButton.h) {
                    // Clear board AND scores
                    scoreX = scoreO = 0;
                    initBoard();
                    firstPlayer = 1;
                    currentPlayer = firstPlayer;
                    needsRedraw = 1;
                    continue;
                }

                // CALCULATES WHICH GRID DOES THE USER CLICK. MAPS TO EACH ROW AND COLUMN
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
                    //ENSURE THAT USER INPUT must be inside a real cell (not gap):
                    if (c>=0 && c<3 && r>=0 && r<3 &&
                        inCellX < CELL_SIZE && inCellY < CELL_SIZE) {

                        if (board[r][c] == EMPTY) {
                            Cell playerPiece = (gameMode==MODE_SP)
                                ? ((playerSide==SIDE_X) ? X : O)
                                : ((currentPlayer==1)?X:O);

                            board[r][c] = playerPiece;
                            currentPlayer = (currentPlayer == 1) ? 2 : 1;

                            // Human just made a move → clear hint + reset timer
                            hintIndex = -1;
                            lastHumanActivityTicks = SDL_GetTicks();

                            needsRedraw = 1;
                        }

                    }
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

        // Hint logic: if AI is one move from winning and human stalls > 5s,
        // highlight the blocking move for the human.
        if (gameMode == MODE_SP) {
            Cell humanPiece = (playerSide == SIDE_X) ? X : O;
            Cell whoseTurnPiece = (currentPlayer == 1) ? X : O;

            // Only consider hint during human's turn and if game not over
            if (whoseTurnPiece == humanPiece && checkWin() == 0 && !isBoardFull()) {
                Uint32 now = SDL_GetTicks();

                if (lastHumanActivityTicks != 0 && (now - lastHumanActivityTicks) > 5000) {
                    int idx = find_blocking_move_against_ai(board, aiPiece);
                    if (idx != hintIndex) {
                        hintIndex = idx;     // can be -1 if no threat
                        needsRedraw = 1;
                    }
                }
            } else {
                // Not human's turn or game finished → clear any previous hint
                if (hintIndex != -1) {
                    hintIndex = -1;
                    needsRedraw = 1;
                }
            }
        }
        //END OF HINT FUNCTION
        if (needsRedraw) renderGame();
        SDL_Delay(16);

        //CALLS THE ANIMATION WIN LINE
        int winner = checkWin();
        if (winner || isBoardFull()) {

            if (winner) {
                    SDL_Color lineColor = (SDL_Color){255, 255, 255, 255};  // pure opaque white
                    animateWinLine(boardRect, lineColor);

            }
            //UPDATE SCORE BOARD
            updateScores(winner);

            if (winner == X)
                displayMessage(gameMode == MODE_SP ? ((playerSide == SIDE_X) ? "You Win!" : "CPU (X) Wins!") : "Player X Wins!");
            else if (winner == O)
                displayMessage(gameMode == MODE_SP ? ((playerSide == SIDE_O) ? "You Win!" : "CPU (O) Wins!") : "Player O Wins!");
            else
                displayMessage("Draw!");

            if (winner == X) firstPlayer = 2;
            else if (winner == O) firstPlayer = 1;

            currentPlayer = firstPlayer;
            initBoard();
            needsRedraw = 1;
            
        }

    }

cleanup:
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit();
    return 0;
}

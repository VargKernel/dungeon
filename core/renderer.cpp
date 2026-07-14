#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <iostream>
#include <string>

#include "renderer.h"
#include "layout.h"
#include "display.h"

// SDL globals
static SDL_Window*   window        = nullptr;
static SDL_Renderer* renderer      = nullptr;
static SDL_Texture*  atlas         = nullptr;
static TTF_Font*     font          = nullptr;
static SDL_Texture*  logicalTarget = nullptr; // pixel-perfect canvas; blitted to the real window in endFrame()
static int           canvasW_      = 0;       // size logicalTarget is currently allocated at
static int           canvasH_      = 0;

/*
Used for both the canvas clear (beginFrame) and the letterbox fill
(endFrame) — sharing one constant means leftover window space always
blends into the canvas instead of showing a seam or a black bar.
*/
static const SDL_Color kBackgroundColor = {13, 43, 69, 255};

// Zoom & camera state
static int zoomLevel = 1;
static int camX      = 0;
static int camY      = 0;

// initSDL
void initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        exit(1);
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
        exit(1);
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << "\n";
        exit(1);
    }

    /*
    Nearest-neighbor filtering — required to keep the pixel-art canvas
    crisp when it's scaled into the real window in endFrame(). The scale
    is a float (not integer-only), so this matters for keeping edges
    clean at non-integer factors.
    */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    /*
    The window starts at the minimum supported resolution (see
    core/display.h's MIN_WINDOW_W/H) — App::run() immediately resizes it
    to the first settings preset anyway, but this keeps the window a
    sane size for the brief moment before that happens. The fixed
    CANVAS_W x CANVAS_H canvas (core/layout.h) scales to fit whatever
    size the window ends up at, so there's no "too small to fit"
    invariant to protect here any more. The window is never resizable by
    dragging; only /set resolution changes its size.
    */
    window = SDL_CreateWindow(
        "Dungeon",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        MIN_WINDOW_W, MIN_WINDOW_H,
        SDL_WINDOW_SHOWN
    );
    if (!window) exit(1);
    SDL_SetWindowMinimumSize(window, MIN_WINDOW_W, MIN_WINDOW_H);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) exit(1);

    atlas = IMG_LoadTexture(renderer, "assets/spritesheet.png");
    if (!atlas) exit(1);

    font = TTF_OpenFont("assets/BigBlueTermPlusNerdFontMono-Regular.ttf", 16);

    /*
    logicalTarget is allocated lazily by the first beginFrame() call —
    simply deferred until then, since it needs `renderer` to already
    exist; its size is always exactly CANVAS_W x CANVAS_H (layout.h).
    */
}

void cleanupSDL() {
    if (logicalTarget) { SDL_DestroyTexture(logicalTarget); logicalTarget = nullptr; }
    if (font)     { TTF_CloseFont(font);            font     = nullptr; }
    if (atlas)    { SDL_DestroyTexture(atlas);      atlas    = nullptr; }
    if (renderer) { SDL_DestroyRenderer(renderer);  renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window);      window   = nullptr; }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

SDL_Window* getWindow() { return window; }

void beginFrame() {
    if (!logicalTarget) {
        logicalTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET,
                                           CANVAS_W, CANVAS_H);
        if (!logicalTarget) exit(1);
        canvasW_ = CANVAS_W;
        canvasH_ = CANVAS_H;
    }

    SDL_SetRenderTarget(renderer, logicalTarget);
    SDL_SetRenderDrawColor(renderer, kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
    SDL_RenderClear(renderer);
}

void getCanvasSize(int& w, int& h) { w = canvasW_; h = canvasH_; }

void endFrame() {
    /*
    Back to the real backbuffer; blit the logical canvas scaled + centered
    (core/display.h's displayComputeDestRect), letterboxed with the same
    background color the canvas itself clears to.
    */
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
    SDL_RenderClear(renderer);

    int windowW = 0, windowH = 0;
    SDL_GetWindowSize(window, &windowW, &windowH);
    SDL_Rect dst = displayComputeDestRect(windowW, windowH);

    SDL_RenderCopy(renderer, logicalTarget, nullptr, &dst);
    SDL_RenderPresent(renderer);
}

void drawChar(TileID c, int x, int y) {
    drawTileRect(c, SDL_Rect{ x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE });
}

void drawTileRect(TileID c, SDL_Rect dst) {
    if (c == EMPTY_ID) return;
    /*
    src's width/height must never exceed what dst actually asked for.
    The map draws every cell one at a time with dst == one CELL_SIZE
    cell — even a multi-tile anchor's own cell — expecting only that
    cell's 1x1 slice; the palette instead passes a dst sized to the
    full meta.w x meta.h sprite and wants the whole region. Clamping
    src to dst (capped by the sheet's real footprint) satisfies both:
    a 1-cell dst still crops 1 cell even for a multi-tile anchor, while
    a full-size dst still gets the entire sprite.
    */
    TileMetadata meta = getTileMeta(c);
    SDL_Rect src = {
        tileX(c) * CELL_SIZE, tileY(c) * CELL_SIZE,
        std::min(dst.w, meta.w * CELL_SIZE),
        std::min(dst.h, meta.h * CELL_SIZE)
    };
    SDL_RenderCopy(renderer, atlas, &src, &dst);
}

void drawString(const std::string& str, int x, int y) {
    drawStringPx(str, x * CELL_SIZE, y * CELL_SIZE);
}

void drawString(const std::string& str, int x, int y, SDL_Color color) {
    drawStringPx(str, x * CELL_SIZE, y * CELL_SIZE, color);
}

void drawStringPx(const std::string& str, int px, int py) {
    drawStringPx(str, px, py, SDL_Color{255, 255, 255, 255});
}

void drawStringPx(const std::string& str, int px, int py, SDL_Color color) {
    if (str.empty() || !font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Solid(font, str.c_str(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect dst = { px, py, surf->w, surf->h };
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void fillRect(int px, int py, int pw, int ph, SDL_Color color) {
    SDL_BlendMode prevBlend;
    SDL_GetRenderDrawBlendMode(renderer, &prevBlend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r = { px, py, pw, ph };
    SDL_RenderFillRect(renderer, &r);

    SDL_SetRenderDrawBlendMode(renderer, prevBlend);
}

void drawRectOutline(int px, int py, int pw, int ph, SDL_Color color) {
    SDL_BlendMode prevBlend;
    SDL_GetRenderDrawBlendMode(renderer, &prevBlend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r = { px, py, pw, ph };
    SDL_RenderDrawRect(renderer, &r);

    SDL_SetRenderDrawBlendMode(renderer, prevBlend);
}

void setClipRect(int px, int py, int pw, int ph) {
    SDL_Rect clip = { px, py, pw, ph };
    SDL_RenderSetClipRect(renderer, &clip);
}

void clearClipRect() {
    SDL_RenderSetClipRect(renderer, nullptr);
}

void setZoom(int level) { zoomLevel = std::clamp(level, 1, 4); }
int getZoom() { return zoomLevel; }
void setMapCamera(int tileX, int tileY) { camX = tileX; camY = tileY; }

static int mapOriginX = 0;
void setMapOrigin(int originX) { mapOriginX = originX; }

void drawMapChar(TileID c, int x, int y) {
    if (c == EMPTY_ID) return;

    SDL_Rect src = { tileX(c) * CELL_SIZE, tileY(c) * CELL_SIZE, CELL_SIZE, CELL_SIZE };

    int tileSize = CELL_SIZE * zoomLevel;
    int screenX, screenY;

    if (zoomLevel == 1) {
        screenX = mapOriginX + x * CELL_SIZE;
        screenY = y * CELL_SIZE;
    } else {
        int camPx = camX * CELL_SIZE + CELL_SIZE / 2;
        int camPy = camY * CELL_SIZE + CELL_SIZE / 2;
        screenX = mapOriginX + (x * CELL_SIZE - camPx) * zoomLevel + MAP_PIXEL_W / 2;
        screenY = (y * CELL_SIZE - camPy) * zoomLevel + MAP_PIXEL_H / 2;
    }

    SDL_Rect dst = { screenX, screenY, tileSize, tileSize };
    SDL_RenderCopy(renderer, atlas, &src, &dst);
}

void setMapClip(bool enable) {
    if (enable) setClipRect(mapOriginX, 0, MAP_PIXEL_W, MAP_PIXEL_H);
    else        clearClipRect();
}

// FrameBuilder
long long FrameBuilder::key(int px, int py) {
    /*
    px/py are always small non-negative pixel coordinates in practice
    (well under 2^20), so packing them into one 64-bit key is exact and
    collision-free — no need for a pair-hash.
    */
    return (static_cast<long long>(py) << 32) | static_cast<unsigned int>(px);
}

void FrameBuilder::markRow(int py, int pxFrom, int pxTo) {
    for (int px = pxFrom; px < pxTo; px += CELL_SIZE) cells_.insert(key(px, py));
}

void FrameBuilder::markCol(int px, int pyFrom, int pyTo) {
    for (int py = pyFrom; py < pyTo; py += CELL_SIZE) cells_.insert(key(px, py));
}

void FrameBuilder::draw() const {
    for (long long k : cells_) {
        int px = static_cast<int>(k & 0xFFFFFFFFLL);
        int py = static_cast<int>(k >> 32);

        bool left  = cells_.count(key(px - CELL_SIZE, py)) != 0;
        bool right = cells_.count(key(px + CELL_SIZE, py)) != 0;
        bool up    = cells_.count(key(px, py - CELL_SIZE)) != 0;
        bool down  = cells_.count(key(px, py + CELL_SIZE)) != 0;

        TileID tile;
        if (left && right && !up && !down)      tile = HORIZONTAL_BORDER;
        else if (up && down && !left && !right) tile = VERTICAL_BORDER;
        else                                    tile = CORNER_BORDER; // corner, T-junction, or cross

        drawTileRect(tile, SDL_Rect{ px, py, CELL_SIZE, CELL_SIZE });
    }
}

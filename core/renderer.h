#pragma once

#include <string>
#include <unordered_set>

#include <SDL2/SDL.h>

#include "tiles.h"

// Initialise SDL, create the game window + renderer, load assets. The
// logical render target is created lazily by the first beginFrame() call.
void initSDL();

// Destroy all SDL resources and shut down subsystems.
void cleanupSDL();

// The single SDL_Window owned by the app. Needed by core/app.cpp to call
// SDL_SetWindowSize when the /set resolution command runs.
SDL_Window* getWindow();

/*
Begin drawing into the logical canvas (not the real backbuffer directly).
Always exactly CANVAS_W x CANVAS_H (core/layout.h) — every mode builds
its frame at this one fixed virtual resolution regardless of the actual
window size; only the FINISHED frame gets scaled, in endFrame(). The
render target is allocated once, lazily, on the first call.
*/
void beginFrame();

/*
The logical canvas size — always CANVAS_W x CANVAS_H. Kept as a function
(rather than having callers just use the layout.h constants directly) so
core/console.cpp doesn't need to depend on layout.h itself.
*/
void getCanvasSize(int& w, int& h);

/*
Blit the logical canvas into the real window — scaled to the largest
integer factor that fits (core/display.h), centered, nearest-neighbor.
Any leftover letterbox space is filled with the same color the canvas
itself clears to, so it reads as part of the interface rather than a
black border.
*/
void endFrame();

// Blit the spritesheet tile that corresponds to TileID `c` at grid cell (x, y).
// Coordinates are raw pixel-grid cells — no zoom applied.
void drawChar(TileID c, int x, int y);

/*
Like drawChar, but with an arbitrary pixel-space destination rect instead
of a CELL_SIZE grid cell. Used by EditorMode for UI that isn't cell-aligned
(the palette preview scales sprites to fit, category tab labels are offset
a few px within their cell, etc). drawChar is a thin wrapper around this.
*/
void drawTileRect(TileID c, SDL_Rect dst);

// Render UTF-8 string `str` at grid cell (x, y) using the game font, in
// plain white.
void drawString(const std::string& str, int x, int y);

// Colored variant — used by the console to tell echoed input, success
// results, and error results apart at a glance (Minecraft-chat style).
void drawString(const std::string& str, int x, int y, SDL_Color color);

// Like drawString, but at an arbitrary pixel position instead of a grid
// cell. drawString is a thin wrapper around this.
void drawStringPx(const std::string& str, int px, int py);
void drawStringPx(const std::string& str, int px, int py, SDL_Color color);

// Fill an axis-aligned rect in raw logical-canvas pixel coordinates (not
// grid cells). Used for UI backgrounds such as the console overlay.
void fillRect(int px, int py, int pw, int ph, SDL_Color color);

// Outline (unfilled) variant of fillRect — used for selection highlights and
// the editor's collision-marker overlay.
void drawRectOutline(int px, int py, int pw, int ph, SDL_Color color);

// Clip subsequent drawing to an arbitrary pixel-space rect, or clear the
// clip.
void setClipRect(int px, int py, int pw, int ph);
void clearClipRect();

/*
Zoom & camera
In-game camera zoom. Unrelated to the interface scale / resolution in
core/display.h — this only affects drawMapChar.
*/

void setZoom(int level);
int  getZoom();

// Update the camera target used by drawMapChar.
void setMapCamera(int tileX, int tileY);

/*
Horizontal pixel offset added to every drawMapChar/setMapClip call —
GameMode reserves a left panel exactly like EditorMode, so the map itself
does not start at canvas x=0. Call once per frame with the current
mapOriginX (core/layout.h's MAP_ORIGIN_X) before drawing (see
game/game_mode.cpp's render()); defaults to 0. EditorMode doesn't use
drawMapChar at all (it draws tiles directly via drawTileRect with its own
explicit offsets), so this has no effect there.
*/
void setMapOrigin(int originX);

// Like drawChar, but zoom- and camera-aware.
void drawMapChar(TileID c, int x, int y);

// Enable / disable the SDL clip rect that confines map rendering to
// (mapOriginX, 0, MAP_PIXEL_W, MAP_PIXEL_H).
void setMapClip(bool enable);


/*
Frame system
FrameBuilder draws panel and divider borders without each mode having to
hand-derive "corner vs edge" itself, and without special-casing where a
panel divider meets the outer border. Mark every cell that should carry
border art (outer edges, plus any interior divider lines), then draw()
decides the correct tile for each marked cell purely from how many of its
4 neighbours are also marked:

  2 opposite neighbours (left+right)  -> HORIZONTAL_BORDER
  2 opposite neighbours (up+down)     -> VERTICAL_BORDER
  anything else (a corner, a T where
  a divider meets the outer edge, or
  a cross where two dividers meet)    -> CORNER_BORDER

CORNER_BORDER's sprite is a full 4-way connector (see assets/spritesheet's
tile (3,33): drawn as both a horizontal AND a vertical bar through the
same cell) — the exact same tile therefore reads correctly as a corner,
a T-junction, or a full cross, so one sprite is all this needs.

Cells are addressed by raw canvas pixel position, not grid index, so a
frame can be docked anywhere.
*/
class FrameBuilder {
public:
    /*
    Marks every CELL_SIZE-aligned cell in the horizontal run
    [pxFrom, pxTo) at height py. pxFrom/pxTo need not be CELL_SIZE-aligned
    themselves — cells are stepped from pxFrom, so a non-aligned run just
    loses its last partial cell rather than throwing off later runs.
    */
    void markRow(int py, int pxFrom, int pxTo);

    // Same as markRow, but a vertical run at a fixed px across [pyFrom, pyTo).
    void markCol(int px, int pyFrom, int pyTo);

    // Computes each marked cell's tile per the rule above and draws it.
    void draw() const;

private:
    static long long key(int px, int py);
    std::unordered_set<long long> cells_;
};

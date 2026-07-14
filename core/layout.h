#pragma once

#include "level.h"

/*
Fixed virtual canvas
The game ALWAYS builds its frame at exactly CANVAS_W x CANVAS_H logical
pixels — the game world, the camera, and every layout in game/editor/
settings mode work purely in these coordinates and never see the real
window size. core/display.h takes that one finished CANVAS_W x CANVAS_H
frame and scales/letterboxes it into whatever window is actually open.
See core/renderer.cpp's beginFrame()/endFrame() and core/app.cpp's run().

Because the canvas size is a compile-time constant rather than a
window-dependent value, every panel/divider/map position below is ALSO a
compile-time constant — there is no per-frame layout computation.
Game/editor/settings modes just reference these constants directly.
*/

constexpr int CELL_SIZE = 16;

constexpr int CANVAS_W = 1280; // fixed virtual resolution — never changes with window size
constexpr int CANVAS_H = 800;

constexpr int UI_BOX_ROWS = 13; // bottom info box, in cells

/*
One dedicated column of UI chrome, used TWICE in EditorMode/GameMode —
once between the left panel and the map, once between the map and the
right panel — so both panels are visually (and spatially) separated from
the map. Never part of either panel's own width and never part of the
map's own MAX_WIDTH cells; see FrameBuilder in core/renderer.h.
*/
constexpr int DIVIDER_W = CELL_SIZE;
constexpr int DIVIDER_H = CELL_SIZE;

constexpr int MAP_PIXEL_W = MAX_WIDTH  * CELL_SIZE;
constexpr int MAP_PIXEL_H = MAX_HEIGHT * CELL_SIZE;

/*
The map's own height plus the info box below it must exactly fill the
canvas height — this is a hard invariant of the shared game/editor
layout, checked at compile time rather than hoped for.
*/
static_assert(MAP_PIXEL_H + UI_BOX_ROWS * CELL_SIZE == CANVAS_H,
              "map + info box rows must exactly fill CANVAS_H");

/*
Both GameMode and EditorMode share the identical three-column layout:
a fixed-width panel, a divider, the fixed-width map, another divider,
then a second fixed-width panel — palette + world list in EditorMode,
reserved/empty in GameMode (see game layout.txt / editor layout.txt).
PANEL_W is whatever's left over after the map and both dividers, split
evenly between the two panels — a pure compile-time constant, since the
canvas itself never grows with the window.
*/
constexpr int PANEL_W = (CANVAS_W - MAP_PIXEL_W - DIVIDER_W * 2) / 2;

// Every side panel, divider, and the map itself must tile the canvas width
// with nothing left over and nothing overlapping.
static_assert(PANEL_W * 2 + DIVIDER_W * 2 + MAP_PIXEL_W == CANVAS_W,
              "left panel + divider + map + divider + right panel must exactly fill CANVAS_W");

/*
Shared column positions (pixels, canvas-space)
The one source of truth for where each region starts — used identically
by game_mode.cpp, editor_mode.cpp, and editor_controls.cpp (rendering AND
hit-testing), so they can never drift apart from one another.
*/
constexpr int LEFT_DIVIDER_X   = PANEL_W;                     // divider between left panel and map
constexpr int MAP_ORIGIN_X     = PANEL_W + DIVIDER_W;         // map's first column
constexpr int MAP_RIGHT_EDGE_X = MAP_ORIGIN_X + MAP_PIXEL_W;  // one past the map's last column
constexpr int RIGHT_PANEL_X    = MAP_RIGHT_EDGE_X + DIVIDER_W; // right panel's first column

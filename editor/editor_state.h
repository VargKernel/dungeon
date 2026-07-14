#pragma once

#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "../core/display.h"
#include "../core/layout.h"
#include "../core/level.h"
#include "../core/renderer.h"
#include "../core/tiles.h"

/*
Shared editor-mode runtime state
editor_mode.cpp owns onEnter()/onRender() (state init + drawing: palette,
map layers, world-location list, info box) and the save/load persistence
helpers; editor_controls.cpp owns onEvent() (keyboard shortcuts, mouse
painting/erasing, scrolling) — see editor_mode.h's EditorMode class. Both
*/
/*
Both files need to read and mutate the same map/palette/selection state, so
it is declared here as extern globals with whole-program storage duration,
reachable from both translation units. editor_mode.cpp is the ONE place
that actually defines (allocates) each of these; every other file only
ever sees the extern declarations below.
*/

constexpr int PALETTE_CELL   = CELL_SIZE;
/*
The palette grid starts at cell (1,1) instead of (0,0) — a one-cell
margin on its top and left, so tiles never sit flush against the panel's
own outer border.
*/
constexpr int PALETTE_MARGIN = CELL_SIZE;

// Editor-local map state
extern TileID edGroundMap   [MAX_HEIGHT][MAX_WIDTH];
extern TileID edObjectMap   [MAX_HEIGHT][MAX_WIDTH];
extern TileID edEntityMap   [MAX_HEIGHT][MAX_WIDTH];
extern TileID edCollisionMap[MAX_HEIGHT][MAX_WIDTH];
extern TileID edOcclusionMap[MAX_HEIGHT][MAX_WIDTH];

// Multitile occupancy tracking
struct MultiTileCell {
    bool occupied;
    int  anchorX;
    int  anchorY;
};
extern MultiTileCell mtMap[MAX_HEIGHT][MAX_WIDTH];

// Tile palette
extern std::vector<TileID> availableTiles;
extern TileID               selectedTile;

// Layer management
enum class EditLayer { GROUND = 1, OBJECTS = 2, ENTITIES = 3, COLLISION = 4, OCCLUSION = 5 };
extern EditLayer activeLayer;

TileID (*getLayerMap(EditLayer layer))[MAX_WIDTH];
EditLayer layerForTile(TileID id);

// Editor session state
extern std::string editorStatus;
extern int         editorStatusTTL;
extern int         paletteScroll;
extern bool        mouseDown_;
extern bool        rightMouseDown_;

/*
World / location selection
selectedCoord is the coordinate F5 saves to and F6 loads from. It's
steppable with the arrow keys (x/y) and PageUp/PageDown (floor) even when
it doesn't correspond to an existing file yet — that's how a brand new
location gets created: step to an empty coordinate, draw, F5. Clicking an
entry in the right-panel list (rendered from listWorldLevels()) jumps
straight to that coordinate instead of stepping to it.
*/
extern LevelCoord selectedCoord;
extern int        worldListScroll; // independent of the palette's paletteScroll

/*
Collision layer carries three different logical markers instead of just
one — which one 1/2/3 places is tracked here. eraseTile doesn't need to
know which; it always just writes EMPTY_ID.
*/
enum class CollisionTool { BLOCK, STAIRS_UP, STAIRS_DOWN };
extern CollisionTool activeCollisionTool;

/*
4 selects the Occlusion layer (see tiles.h's OCCLUSION_MARKER) — one
marker per click, exactly like COLLISION_MARKER/STAIRS_*_MARKER; a 3-tall
doorway occlusion zone is 3 separate clicks. Entirely invisible in
GameMode; the editor draws a translucent outline over marked cells so
they're still something to aim at — see editor_mode.cpp's collision/
occlusion overlay and editor_controls.cpp's placeTile/eraseTile.
*/

/*
Palette layout (shelf packing)
Tiles are packed left-to-right into "shelves": a shelf's height is the
tallest tile placed in it, and a tile starts a new shelf only when it no
longer fits the remaining width of the current one. Rects are in
content-space — PALETTE_MARGIN-relative, scroll NOT yet applied — so the
same layout can be reused for both rendering (editor_mode.cpp, subtract
paletteScroll) and hit-testing (editor_controls.cpp, add paletteScroll).
*/
struct PaletteEntry { TileID id; SDL_Rect rect; };
std::vector<PaletteEntry> computePaletteLayout(int panelW);

/*
Painting actions
Mutate the ed*Map/mtMap state above — implemented in editor_controls.cpp,
since only mouse input (handlePointerAction) ever calls them.
*/
void placeTile(int gx, int gy);
void eraseTile(int gx, int gy);

/*
Persistence
F5/F6 — implemented in editor_mode.cpp (alongside clearEditorMaps/
rebuildMultiTileOccupancy, which they share with onEnter()'s startup
load), called from editor_controls.cpp's onEvent().
*/
void saveEditorToLocation(const LevelCoord& coord);
void loadLocationIntoEditor(const LevelCoord& coord);

#include "editor_mode.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/display.h"
#include "../core/layout.h"
#include "../core/level.h"
#include "../core/renderer.h"
#include "../core/tiles.h"
#include "editor_panel.h"
#include "editor_state.h"

/*
Editor-local map state
Defined here (the ONE translation unit that allocates them); declared
`extern` in editor_state.h for editor_controls.cpp to reach.
*/
TileID edGroundMap   [MAX_HEIGHT][MAX_WIDTH];
TileID edObjectMap   [MAX_HEIGHT][MAX_WIDTH];
TileID edEntityMap   [MAX_HEIGHT][MAX_WIDTH];
TileID edCollisionMap[MAX_HEIGHT][MAX_WIDTH];
TileID edOcclusionMap[MAX_HEIGHT][MAX_WIDTH];
MultiTileCell mtMap[MAX_HEIGHT][MAX_WIDTH];

// Tile palette
std::vector<TileID> availableTiles;
TileID               selectedTile = makeTile(1, 2);

// Layer management
EditLayer activeLayer = EditLayer::GROUND;

TileID (*getLayerMap(EditLayer layer))[MAX_WIDTH] {
    switch (layer) {
        case EditLayer::GROUND:    return edGroundMap;
        case EditLayer::OBJECTS:   return edObjectMap;
        case EditLayer::ENTITIES:  return edEntityMap;
        case EditLayer::COLLISION: return edCollisionMap;
        case EditLayer::OCCLUSION: return edOcclusionMap;
    }
    return edGroundMap;
}

EditLayer layerForTile(TileID id) {
    switch (getTileMeta(id).layer) {
        case LayerType::Ground:    return EditLayer::GROUND;
        case LayerType::Objects:   return EditLayer::OBJECTS;
        case LayerType::Entities:  return EditLayer::ENTITIES;
        case LayerType::Collision: return EditLayer::COLLISION;
    }
    return EditLayer::GROUND;
}

// Editor session state
std::string editorStatus    = "";
int         editorStatusTTL = 0;
int         paletteScroll   = 0;
bool        mouseDown_      = false;
bool        rightMouseDown_ = false;

// World / location selection
LevelCoord    selectedCoord{0, 0, 0};
int           worldListScroll = 0;
CollisionTool activeCollisionTool = CollisionTool::BLOCK;

/*
buildPalette
No scanning of assets/spritesheet.png for non-transparent cells — the
palette is built entirely from tiles.cpp's hand-maintained registry (see
core/tiles.h's getPaletteTiles()). Adding a new paintable tile is a
one-line addition there; this function needs no changes for it.
*/
static void buildPalette() {
    availableTiles = getPaletteTiles();
    if (!availableTiles.empty()) selectedTile = availableTiles[0];
}

/*
Palette layout (shelf packing)
See editor_state.h's PaletteEntry/computePaletteLayout comment — this is
the one implementation, shared by onRender() below (drawing) and
editor_controls.cpp's handlePointerAction (click hit-testing).
*/
std::vector<PaletteEntry> computePaletteLayout(int panelW) {
    std::vector<PaletteEntry> entries;
    entries.reserve(availableTiles.size());

    const int availW = panelW - PALETTE_MARGIN;
    int cursorX = 0, cursorY = 0, shelfH = 0;

    for (TileID id : availableTiles) {
        TileMetadata meta = getTileMeta(id);
        int w = meta.w * PALETTE_CELL;
        int h = meta.h * PALETTE_CELL;

        /*
        Wrap to a new shelf if this tile doesn't fit what's left of the
        current one — but never wrap a shelf that's still empty (a tile
        wider than the whole panel just overflows rather than looping).
        */
        if (cursorX > 0 && cursorX + w > availW) {
            cursorX = 0;
            cursorY += shelfH;
            shelfH = 0;
        }

        entries.push_back({ id, SDL_Rect{ PALETTE_MARGIN + cursorX, PALETTE_MARGIN + cursorY, w, h } });

        cursorX += w;
        shelfH = std::max(shelfH, h);
    }

    return entries;
}

/*
clearEditorMaps
A genuinely empty canvas — no border, no default content of any kind.
Used both for a fresh editor session and as F6's fallback when the
selected coordinate has no saved file yet.
*/
static void clearEditorMaps() {
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            edGroundMap[y][x]    = EMPTY_ID;
            edObjectMap[y][x]    = EMPTY_ID;
            edEntityMap[y][x]    = EMPTY_ID;
            edCollisionMap[y][x] = EMPTY_ID;
            edOcclusionMap[y][x] = EMPTY_ID;
            mtMap[y][x]          = { false, 0, 0 };
        }
    }
}

/*
rebuildMultiTileOccupancy
mtMap (which cells belong to which multi-tile anchor) is editor-only state
that never gets saved to JSON — a loaded map only has raw TileIDs. After
F6 loads a file into ed*Map, this reconstructs mtMap by scanning for
anchor tiles: since only a multi-tile sprite's own top-left coordinate is
ever registered in tiles.cpp's registry with w>1/h>1 (its sub-cells hold
different, unregistered TileIDs that fall back to {1,1}), any cell whose
stored ID resolves to a w>1/h>1 meta IS an anchor, unambiguously — so this
needs no guessing beyond what getTileMeta already encodes.
*/
static void rebuildMultiTileOccupancy() {
    for (int y = 0; y < MAX_HEIGHT; ++y)
        for (int x = 0; x < MAX_WIDTH; ++x)
            mtMap[y][x] = { false, 0, 0 };

    auto scanLayer = [](TileID map[][MAX_WIDTH]) {
        for (int y = 0; y < MAX_HEIGHT; ++y) {
            for (int x = 0; x < MAX_WIDTH; ++x) {
                TileID id = map[y][x];
                if (id == EMPTY_ID) continue;

                TileMetadata meta = getTileMeta(id);
                if (meta.w <= 1 && meta.h <= 1) continue;

                for (int dy = 0; dy < meta.h; ++dy) {
                    for (int dx = 0; dx < meta.w; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int cx = x + dx, cy = y + dy;
                        if (cx < MAX_WIDTH && cy < MAX_HEIGHT)
                            mtMap[cy][cx] = { true, x, y };
                    }
                }
            }
        }
    };
    scanLayer(edGroundMap);
    scanLayer(edObjectMap);
    scanLayer(edEntityMap);
}

/*
loadLocationIntoEditor
F6: loads `coord` into the ed*Map layers. If nothing is saved there yet,
clears to a genuinely empty canvas — no fallback placeholder/border
content — so a not-yet-existing coordinate is still something to draw on
rather than a dead end.
*/
void loadLocationIntoEditor(const LevelCoord& coord) {
    Level* level = findLevel(coord.floor, coord.x, coord.y);
    if (!level) {
        clearEditorMaps();
        editorStatus    = "No saved map at this location \u2014 empty canvas";
        editorStatusTTL = 150;
        return;
    }

    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            edGroundMap[y][x]    = level->tileMap[y][x];
            edObjectMap[y][x]    = level->objectMap[y][x];
            edEntityMap[y][x]    = level->entityMap[y][x];
            edCollisionMap[y][x] = level->collisionMap[y][x];
            edOcclusionMap[y][x] = level->occlusionMap[y][x];
        }
    }
    rebuildMultiTileOccupancy();

    editorStatus    = std::string("Loaded \"") + level->name + "\"";
    editorStatusTTL = 150;
}

/*
saveEditorToLocation
F5: writes the ed*Map layers out to `coord`'s JSON file in world/, and
registers/updates the in-memory copy in worldLevels immediately — so the
right-panel list and console /load autocomplete see it without needing a
restart.
*/
void saveEditorToLocation(const LevelCoord& coord) {
    Level& level = getOrCreateLevel(coord.floor, coord.x, coord.y);

    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            level.tileMap[y][x]      = edGroundMap[y][x];
            level.objectMap[y][x]    = edObjectMap[y][x];
            level.entityMap[y][x]    = edEntityMap[y][x];
            level.collisionMap[y][x] = edCollisionMap[y][x];
            level.occlusionMap[y][x] = edOcclusionMap[y][x];
        }
    }
    level.width  = MAX_WIDTH;
    level.height = MAX_HEIGHT;
    level.floor  = coord.floor;
    level.locX   = coord.x;
    level.locY   = coord.y;
    if (strcmp(level.name, "New Location") == 0)
        snprintf(level.name, sizeof(level.name), "Location %d/%d/%d", coord.floor, coord.x, coord.y);

    bool ok = saveLevelToFile(level, levelFileName(coord.floor, coord.x, coord.y).c_str());

    editorStatus    = ok ? (std::string(" Saved \"") + level.name + "\" to " + levelFileName(coord.floor, coord.x, coord.y))
                          : "Save FAILED \u2014 check world/ is writable";
    editorStatusTTL = 150;
}

/*
drawInfoStr
Info-box text helper (relative to map area, cell-grid coords). originX is
the pixel x-origin to offset from — MAP_ORIGIN_X for the map's own info
box, RIGHT_PANEL_X for the world-location list (see core/layout.h).
*/
static void drawInfoStr(const std::string& text, int gx, int gy, int originX) {
    drawStringPx(text, originX + gx * CELL_SIZE, gy * CELL_SIZE);
}

/*
IMode
Same default as GameMode::onEnter(): tries to load (floor=0, x=0, y=0) —
selectedCoord's initial value — and if nothing is saved there, leaves the
canvas genuinely empty (see loadLocationIntoEditor) rather than seeding it
with any placeholder/template content.
*/
void EditorMode::onEnter() {
    buildPalette();
    loadLocationIntoEditor(selectedCoord);
}

void EditorMode::onRender() {
    /*
    App::beginFrame() already cleared the canvas to the background color —
    nothing to do here. All geometry below comes straight from
    core/layout.h's compile-time constants — the canvas is always exactly
    CANVAS_W x CANVAS_H, so there's nothing left to compute per frame (see
    layout.h's header comment). panelContentW applies to BOTH the left
    (palette) and right (world-location list) panels — they are always
    identical by construction (PANEL_W).
    */
    const int panelH         = CANVAS_H;

    const int mapOriginX     = MAP_ORIGIN_X;
    const int mapRightEdgeX  = MAP_RIGHT_EDGE_X; // one past the map's last column

    /*
    Each side panel has its OWN dedicated DIVIDER_W column for the wall
    that separates it from the map, instead of that wall coinciding with
    (and being painted over) either the map's own first/last column or
    the panel's own clip rect. panelContentW (== PANEL_W, < mapOriginX)
    keeps both panels visually identical widths and keeps the map's own
    edge columns unobstructed. See layout.h's DIVIDER_W / LEFT_DIVIDER_X.
    */
    const int panelContentW = PANEL_W;
    const int leftDividerX  = LEFT_DIVIDER_X;
    const int rightPanelX   = RIGHT_PANEL_X;

    const int totalW = CANVAS_W;
    const int totalH = CANVAS_H; // == panelH — the info box below the map fills all the way down to it too

    /*
    Palette
    The entire left panel, top to bottom — no category bar or preview box
    carving out space above it anymore. Starts at cell (1,1): a one-cell
    margin along the top and left keeps tiles clear of the panel's own
    border instead of sitting flush against it.
    */
    {
        setClipRect(0, 0, panelContentW, panelH);

        for (const PaletteEntry& entry : computePaletteLayout(panelContentW)) {
            const SDL_Rect& r = entry.rect;
            int px = r.x;
            int py = r.y - paletteScroll;

            if (py + r.h > 0 && py < panelH)
                drawTileRect(entry.id, SDL_Rect{px, py, r.w, r.h});

            if (entry.id == selectedTile)
                drawRectOutline(px, py, r.w, r.h, SDL_Color{255, 255, 0, 255});
        }

        clearClipRect();
    }

    /*
    Right panel: world location list
    The F5/F6 target (selectedCoord) plus every location already saved to
    world/, so it doubles as both a picker (click a row, or arrow keys /
    PageUp/PageDown to reach a coordinate that isn't saved yet) and a
    status readout of what's on disk right now.
    */
    {
        setClipRect(rightPanelX, 0, panelContentW, panelH);

        std::vector<LevelCoord> coords = listWorldLevels();
        bool selectedExists = false;
        for (const LevelCoord& c : coords)
            if (c.floor == selectedCoord.floor && c.x == selectedCoord.x && c.y == selectedCoord.y)
                selectedExists = true;

        std::string header = "Floor " + std::to_string(selectedCoord.floor)
            + " (" + std::to_string(selectedCoord.x) + ", " + std::to_string(selectedCoord.y) + ")"
            + (selectedExists ? "" : " [new]");
        drawInfoStr(header, 0, EditorPanel::ROW_WORLD_HEADER, rightPanelX);
        drawInfoStr(EditorPanel::WORLD_LEGEND, 0, EditorPanel::ROW_WORLD_LEGEND, rightPanelX);

        for (size_t i = 0; i < coords.size(); ++i) {
            const LevelCoord& c = coords[i];
            int py = PALETTE_MARGIN + CELL_SIZE * (EditorPanel::WORLD_LIST_START_ROW + (int)i) - worldListScroll;
            if (py + CELL_SIZE <= 0 || py >= panelH) continue;

            bool isSelected = (c.floor == selectedCoord.floor && c.x == selectedCoord.x && c.y == selectedCoord.y);
            const Level* level = findLevel(c.floor, c.x, c.y);

            std::string row = " " + std::to_string(c.floor) + "/" + std::to_string(c.x) + "/" + std::to_string(c.y)
                + " " + (level ? level->name : "?");

            SDL_Color color = isSelected ? SDL_Color{255, 255, 0, 255} : SDL_Color{200, 200, 200, 255};
            if (isSelected)
                drawRectOutline(rightPanelX, py, panelContentW, CELL_SIZE, SDL_Color{255, 255, 0, 255});
            drawStringPx(row, rightPanelX, py, color);
        }

        clearClipRect();
    }

    // Map layers (Ground → Objects → Entities, matches game render order)
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            int px = mapOriginX + x * CELL_SIZE;
            int py = y * CELL_SIZE;

            drawTileRect(edGroundMap[y][x], SDL_Rect{px, py, CELL_SIZE, CELL_SIZE});
            drawTileRect(edObjectMap[y][x], SDL_Rect{px, py, CELL_SIZE, CELL_SIZE});
            drawTileRect(edEntityMap[y][x], SDL_Rect{px, py, CELL_SIZE, CELL_SIZE});
        }
    }

    /*
    Collision overlay
    Three distinct marker colors: yellow = blocking wall (TAB), cyan =
    stairs up (5), magenta = stairs down (6) — matches whichever tool is
    currently active, so what you're about to place is always visible.
    */
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            TileID marker = edCollisionMap[y][x];
            SDL_Color color;
            if      (marker == COLLISION_MARKER)   color = SDL_Color{255, 220, 0, 200};
            else if (marker == STAIRS_UP_MARKER)   color = SDL_Color{80, 220, 255, 200};
            else if (marker == STAIRS_DOWN_MARKER) color = SDL_Color{230, 90, 255, 200};
            else continue;

            drawRectOutline(mapOriginX + x * CELL_SIZE, y * CELL_SIZE,
                             CELL_SIZE, CELL_SIZE, color);
        }
    }

    /*
    Occlusion overlay
    Invisible in GameMode (see tiles.h's OCCLUSION_MARKER) — drawn here as
    a translucent violet outline purely so there's something to aim at
    while placing/erasing it (O key — see editor_controls.cpp).
    */
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        for (int x = 0; x < MAX_WIDTH; ++x) {
            if (edOcclusionMap[y][x] != OCCLUSION_MARKER) continue;
            drawRectOutline(mapOriginX + x * CELL_SIZE, y * CELL_SIZE,
                             CELL_SIZE, CELL_SIZE, SDL_Color{170, 100, 255, 200});
        }
    }

    /*
    Frame
    The map's info box (the text area below the tile grid) is not a
    separately-boxed, fixed-height region — it fills the rest of the
    canvas, exactly like the side panels do, so there is no divider row
    between the map and the info box: the map's own left/right walls run
    the full canvas height.

    The top and bottom are each a SINGLE continuous row spanning the entire
    canvas width — palette, gap, map, gap, reserved panel, all in one line —
    rather than separate per-region segments. This needs no special-casing
    to avoid corners: FrameBuilder's neighbor detection renders a cell as a
    corner only where a vertical wall also meets it (the real corners of
    the palette/map/reserved-panel boxes), and as plain horizontal
    everywhere else — including the cell bridging each gap, since nothing
    vertical is ever marked there.
    */
    {
        FrameBuilder fb;
        fb.markRow(0, 0, totalW);
        fb.markRow(totalH - CELL_SIZE, 0, totalW);
        fb.markCol(0, 0, totalH);
        fb.markCol(totalW - CELL_SIZE, 0, totalH);
        fb.markCol(leftDividerX, 0, totalH);
        fb.markCol(mapRightEdgeX, 0, totalH);
        /*
        Separates the map viewport from the info box below it — only
        spans the map's own width (mapOriginX..mapRightEdgeX), not the
        full canvas, since the left/right panels keep running uninterrupted
        top to bottom. Deliberately drawn one row up (MAX_HEIGHT - 1, not
        MAX_HEIGHT), so it overlaps the map's own last row rather than
        sitting flush below it — intentional, not an off-by-one.
        */
        fb.markRow((MAX_HEIGHT - 1) * CELL_SIZE, mapOriginX, mapRightEdgeX);
        fb.draw();
    }

    /*
    Info box text
    Row indices come from editor_panel.h — see that file for the panel's
    row budget (max 12, rows 0..11).
    */
    const int boxStartY = MAX_HEIGHT + 2;
    {
        if (editorStatusTTL > 0) {
            drawInfoStr(editorStatus, 1, boxStartY + EditorPanel::ROW_STATUS, mapOriginX);
            --editorStatusTTL;
        }

        drawInfoStr(EditorPanel::HELP_PLACE,     1, boxStartY + EditorPanel::ROW_HELP_PLACE,     mapOriginX);
        drawInfoStr(EditorPanel::HELP_COLLISION, 1, boxStartY + EditorPanel::ROW_HELP_COLLISION, mapOriginX);
        drawInfoStr(EditorPanel::HELP_LOCATION,  1, boxStartY + EditorPanel::ROW_HELP_LOCATION,  mapOriginX);
        drawInfoStr(EditorPanel::HELP_SAVE,      1, boxStartY + EditorPanel::ROW_HELP_SAVE,      mapOriginX);
        drawInfoStr(EditorPanel::HELP_QUIT,      1, boxStartY + EditorPanel::ROW_HELP_QUIT,      mapOriginX);
    }
}

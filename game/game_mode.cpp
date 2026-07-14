#include "game_mode.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "../core/display.h"
#include "../core/layout.h"
#include "../core/level.h"
#include "../core/renderer.h"
#include "../core/tiles.h"
#include "game_panel.h"
#include "game_state.h"
#include "interactions.h"

/*
Runtime layer arrays
Defined here (the ONE translation unit that allocates them); declared
`extern` in game_state.h for game_controls.cpp to reach. Same
whole-program storage duration file-scope statics had before — this
preserves the original's behavior of surviving a round trip through
EditorMode (recreated as a fresh GameMode instance).
*/
TileID gTileLayer     [MAX_HEIGHT][MAX_WIDTH];
TileID gObjectLayer   [MAX_HEIGHT][MAX_WIDTH];
TileID gEntityLayer   [MAX_HEIGHT][MAX_WIDTH];
TileID gCollisionLayer[MAX_HEIGHT][MAX_WIDTH];
TileID gOcclusionMap  [MAX_HEIGHT][MAX_WIDTH];

// Player state
int    gPlayerPosX  = 0;
int    gPlayerPosY  = 0;
Facing gPlayerFacing = Facing::Down;

void facingCell(int& fx, int& fy) {
    fx = gPlayerPosX;
    fy = gPlayerPosY;
    switch (gPlayerFacing) {
        case Facing::Up:    fy--; break;
        case Facing::Down:  fy++; break;
        case Facing::Left:  fx--; break;
        case Facing::Right: fx++; break;
    }
}

// Global game state
LevelCoord gCurrentCoord{0, 0, 0};
Level*     gCurrentLevel = nullptr;

/*
Object interaction
The actual "what does tile X do" data lives in game/interactions.h/.cpp —
this file only asks findInteraction() what (if anything) a control does to
whatever's at the facing cell (game_controls.cpp carries that out), then
replaceFootprint below stamps the tile change and playAnimation, further
down, steps through it frame by frame with a visible delay.
*/

/*
Stamps newAnchor's full w x h footprint into gObjectLayer starting at
(ax, ay), first clearing whichever of the old/new footprints is larger.
Mirrors EditorMode's placeTile: each covered cell gets its own sub-tile
(tileX(newAnchor)+dx, tileY(newAnchor)+dy), not a copy of the anchor —
that's what makes "is gObjectLayer[y][x] a known anchor tile" a reliable
lookup key elsewhere: an untouched anchor cell always literally equals
whatever TileID it was placed with, dx=dy=0 leaves it unchanged.
*/
static void replaceFootprint(int ax, int ay, TileID oldAnchor, TileID newAnchor) {
    TileMetadata oldMeta = getTileMeta(oldAnchor);
    TileMetadata newMeta = getTileMeta(newAnchor);
    int clearW = std::max(oldMeta.w, newMeta.w);
    int clearH = std::max(oldMeta.h, newMeta.h);

    for (int dy = 0; dy < clearH; dy++)
        for (int dx = 0; dx < clearW; dx++) {
            int cx = ax + dx, cy = ay + dy;
            if (cx < MAX_WIDTH && cy < MAX_HEIGHT) gObjectLayer[cy][cx] = EMPTY_ID;
        }

    for (int dy = 0; dy < newMeta.h; dy++)
        for (int dx = 0; dx < newMeta.w; dx++) {
            int cx = ax + dx, cy = ay + dy;
            if (cx < MAX_WIDTH && cy < MAX_HEIGHT)
                gObjectLayer[cy][cx] = makeTile(tileX(newAnchor) + dx, tileY(newAnchor) + dy);
        }
}

// HUD strings
std::string gLevelName    = "";
std::string gGameMessage  = "";

/*
Layout
Same fixed three-column layout as EditorMode — panel/divider/map/divider/
panel positions all come straight from core/layout.h's compile-time
constants (PANEL_W, MAP_ORIGIN_X, ...) since the canvas itself is a fixed
CANVAS_W x CANVAS_H (see layout.h's header comment). GameMode's panels are
reserved/empty for now — see drawReservedPanel() below — but the geometry
is pixel-for-pixel the same as the editor's.
*/

/*
Pixel-space text helper relative to an arbitrary panel origin — identical
in spirit to EditorMode's drawInfoStr, just under a name that doesn't
imply "info box" (GameMode uses it for the info box AND the reserved
panels alike).
*/
static void drawAt(const std::string& text, int gx, int gy, int originX) {
    drawStringPx(text, originX + gx * CELL_SIZE, gy * CELL_SIZE);
}

/*
A panel's line list — border comes from the shared FrameBuilder pass in
render(), this just draws each entry on its own row starting at
startRow (left/right panels start at row 1, leaving row 0 as a top
margin; the info box starts at boxStartY + 0 — see its call site).
startCol is the horizontal indent in cells — 1 everywhere except the
right panel, which draws flush against its own left edge (startCol 0).
*/
static void drawPanelLines(const std::vector<std::string>& lines, int originX, int startRow = 1, int startCol = 1) {
    for (size_t i = 0; i < lines.size(); ++i)
        drawAt(lines[i], startCol, startRow + static_cast<int>(i), originX);
}

// initMap
static void initMap(Level& level, const LevelEntry& entry) {
    /*
    Tracks whether gPlayerPosX/Y have actually been set to something that
    belongs to THIS level (either its own authored PLAYER spawn, or a
    matching stairs/edge marker below) — without this, a level entered via
    FromAbove/FromBelow that's missing the matching stairs marker (e.g. a
    freshly-created floor nobody's placed the return stairs on yet) left
    gPlayerPosX/Y at whatever the PREVIOUS level's coordinates happened to
    be: the player didn't visually move at all (same coordinates, and no
    PLAYER sprite drawn there since the new entityMap rarely has one at
    that exact stale cell either) — looked exactly like stepping on the
    stairs did nothing.
    */
    bool playerPlaced = false;

    for (int y = 0; y < level.height; y++) {
        for (int x = 0; x < level.width; x++) {
            gTileLayer[y][x] = level.tileMap[y][x];

            // objectMap holds EMPTY_ID for empty cells (set by initLevels);
            // the zero-check is a defensive fallback for direct struct init.
            gObjectLayer[y][x] = (level.objectMap[y][x] != 0) ? level.objectMap[y][x] : EMPTY_ID;

            /*
            collisionMap stores logical markers only (COLLISION_MARKER /
            STAIRS_UP_MARKER / STAIRS_DOWN_MARKER); strip any non-marker
            visual tiles that might otherwise have ended up in there.
            */
            TileID col = level.collisionMap[y][x];
            if (col != COLLISION_MARKER && col != STAIRS_UP_MARKER && col != STAIRS_DOWN_MARKER)
                col = EMPTY_ID;
            gCollisionLayer[y][x] = col;

            gOcclusionMap[y][x] = level.occlusionMap[y][x];

            gEntityLayer[y][x] = level.entityMap[y][x];

            if (gEntityLayer[y][x] == PLAYER) {
                gPlayerPosX  = x;
                gPlayerPosY  = y;
                playerPlaced = true;
            }
        }
    }

    auto placePlayer = [&](int x, int y) {
        gEntityLayer[gPlayerPosY][gPlayerPosX] = EMPTY_ID;
        gPlayerPosX  = x;
        gPlayerPosY  = y;
        gEntityLayer[y][x] = PLAYER;
        playerPlaced = true;
    };

    if (entry.kind == LevelEntry::Kind::FromAbove) {
        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++)
                if (gCollisionLayer[y][x] == STAIRS_UP_MARKER) placePlayer(x, y);
    }
    else if (entry.kind == LevelEntry::Kind::FromBelow) {
        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++)
                if (gCollisionLayer[y][x] == STAIRS_DOWN_MARKER) placePlayer(x, y);
    }
    else if (entry.kind == LevelEntry::Kind::EdgeWest) {
        /*
        Crossed the border heading east — appear one cell inside the new
        map's west edge (column 1, not column 0 — column 0 is itself a
        crossing trigger, see game_controls.cpp's edge-crossing check, so
        landing exactly on it would risk immediately re-triggering on the
        very next same-direction step).
        */
        placePlayer(1, std::clamp(entry.carry, 0, level.height - 1));
    }
    else if (entry.kind == LevelEntry::Kind::EdgeEast) {
        placePlayer(level.width - 2, std::clamp(entry.carry, 0, level.height - 1));
    }
    else if (entry.kind == LevelEntry::Kind::EdgeNorth) {
        // Crossed the border heading south — appear one cell inside the new
        // map's north edge (row 1, not row 0 — same reasoning as EdgeWest).
        placePlayer(std::clamp(entry.carry, 0, level.width - 1), 1);
    }
    else if (entry.kind == LevelEntry::Kind::EdgeSouth) {
        placePlayer(std::clamp(entry.carry, 0, level.width - 1), level.height - 2);
    }
    // Kind::Default: keep whatever position the entityMap scan above found.

    /*
    Nothing above ever set a position that belongs to this level (no
    authored spawn, and — for FromAbove/FromBelow — no matching stairs
    marker was found here to land on). Fall back to (0,0) rather than
    silently keeping the previous level's coordinates.
    */
    if (!playerPlaced) placePlayer(0, 0);

    setMapCamera(gPlayerPosX, gPlayerPosY);
}

/*
enterLevel
Central place that switches the active location: looks it up (creating it
blank first only via the editor, never here — a missing neighbor simply
isn't traversable), loads its layers, and updates the HUD title.
*/
bool enterLevel(int floor, int x, int y, const LevelEntry& entry) {
    Level* level = findLevel(floor, x, y);
    if (!level) return false;

    gCurrentCoord = LevelCoord{floor, x, y};
    gCurrentLevel = level;
    initMap(*gCurrentLevel, entry);
    gLevelName = gCurrentLevel->name;
    return true;
}

// render
void GameMode::render() {
    // Fixed geometry — see core/layout.h. The canvas is always exactly
    // CANVAS_W x CANVAS_H, so none of this needs per-frame computation.
    const int panelH         = CANVAS_H;
    const int mapOriginX     = MAP_ORIGIN_X;
    const int mapRightEdgeX  = MAP_RIGHT_EDGE_X;
    const int panelContentW  = PANEL_W;
    const int leftDividerX   = LEFT_DIVIDER_X;
    const int rightPanelX    = RIGHT_PANEL_X;

    const int totalW = CANVAS_W;
    const int totalH = CANVAS_H;

    /*
    Side panels
    Left holds location + the stat block (see GamePanel::buildLeftPanelLines);
    right is still plain placeholder content (GamePanel::RIGHT_PANEL_LINES).
    Drawn unconditionally, same as the frame below — a missing location (no
    file at 0,0,0 yet) only means an empty MAP, not a broken screen.
    */
    {
        std::vector<std::string> statLines = {
            GamePanel::buildStatLine(GamePanel::ICON_HP,  "HP ", "[hp] "),
            GamePanel::buildStatLine(GamePanel::ICON_STA, "STA", "[sta]"),
            GamePanel::buildStatLine(GamePanel::ICON_MP,  "MP ", "[mp] "),
            GamePanel::buildStatLine(GamePanel::ICON_ATK, "ATK", "[atk]"),
            GamePanel::buildStatLine(GamePanel::ICON_DEF, "DEF", "[def]"),
            GamePanel::buildStatLine(GamePanel::ICON_DEX, "DEX", "[dex]"),
            GamePanel::buildStatLine(GamePanel::ICON_INT, "INT", "[int]"),
            GamePanel::buildStatLine(GamePanel::ICON_CHA, "CHA", "[cha]"),
            GamePanel::buildStatLine(GamePanel::ICON_LCK, "LCK", "[lck]"),
        };
        std::vector<std::string> leftLines = GamePanel::buildLeftPanelLines(
            gLevelName, statLines,
            GamePanel::ZOOM_LABEL_PREFIX + std::to_string(getZoom()) + GamePanel::ZOOM_LABEL_SUFFIX);

        setClipRect(0, 0, panelContentW, panelH);
        drawPanelLines(leftLines, 0);
        clearClipRect();
    }
    {
        setClipRect(rightPanelX, 0, panelContentW, panelH);
        drawPanelLines(GamePanel::RIGHT_PANEL_LINES, rightPanelX, 1, 0);
        clearClipRect();
    }

    /*
    Map
    Nothing to draw if no location is loaded (see onEnter(): there's no
    placeholder fallback any more) — the map area is simply left empty,
    same as a freshly-opened EditorMode with nothing saved yet.
    */
    setMapOrigin(mapOriginX);
    setMapClip(true);

    if (gCurrentLevel) {
        const Level& level = *gCurrentLevel;

        // Render order: Ground → Objects → Entities
        // Collision layer is logical only and is NOT drawn in the game view.
        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++)
                if (gTileLayer[y][x] != EMPTY_ID)
                    drawMapChar(gTileLayer[y][x], x, y);

        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++)
                if (gObjectLayer[y][x] != EMPTY_ID)
                    drawMapChar(gObjectLayer[y][x], x, y);

        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++)
                if (gEntityLayer[y][x] != EMPTY_ID)
                    drawMapChar(gEntityLayer[y][x], x, y);

        /*
        Ground + Object cells redrawn over the Entities layer (in that
        same order, so Objects still sits above Ground like normal),
        wherever an entity (the player) happens to be standing on a cell
        the editor marked as an occlusion zone (see editor_state.h's
        OCCLUSION layer / tiles.h's OCCLUSION_MARKER) — makes walking
        into a doorway (or any other marked archway) read as passing
        UNDER/THROUGH it instead of standing in front of it, regardless
        of which layer the archway's sprite actually lives on (some
        "door frame" wall sprites are Ground, not Objects — see
        tiles.cpp's registry). Purely data-driven: no door-specific tile
        IDs are checked here at all — whatever the level author marked
        is what occludes.
        */
        for (int y = 0; y < level.height; y++)
            for (int x = 0; x < level.width; x++) {
                if (gEntityLayer[y][x] == EMPTY_ID || gOcclusionMap[y][x] != OCCLUSION_MARKER) continue;
                drawMapChar(gTileLayer[y][x], x, y);
                drawMapChar(gObjectLayer[y][x], x, y);
            }

        /*
        Facing cursor — drawn last so it sits on top of everything else,
        marking whichever cell the E key will act on this frame.
        Horizontally, fx == -1 / fx == level.width are the only excluded
        values — column 0 and column width - 1 are genuine walkable tiles
        (see game_controls.cpp's edge-crossing bounds), not covered by any
        frame chrome. Vertically it's the opposite: row 0 sits exactly
        under the outer frame's top row, and row height - 1 sits exactly
        under the map/info-box divider row (see the Frame section below —
        both deliberately overlap those tile rows rather than sitting
        flush outside them), so the facing indicator would silently
        vanish under the border there. Row 0 and row height - 1 are
        excluded here for that reason, even though they're valid array
        indices — the player never actually stands there either (see
        game_controls.cpp: the vertical edge-crossing ring is exactly one
        cell in from the array bounds, unlike the horizontal one).
        */
        int fx, fy;
        facingCell(fx, fy);
        if (fx >= 0 && fx < level.width && fy >= 1 && fy < level.height - 1)
            drawMapChar(FACING_INDICATOR, fx, fy);
    }

    setMapClip(false);

    const int mapRows = gCurrentLevel ? gCurrentLevel->height : MAX_HEIGHT;

    /*
    Frame
    Pixel-for-pixel the same shape as EditorMode's: one continuous outer
    frame from the top of the canvas to the bottom, with a vertical wall
    wherever the left panel, the map, or the right panel begins/ends, PLUS
    one horizontal divider — only as wide as the map itself — separating
    the map viewport from the info box below it. Deliberately drawn one
    row up (mapRows - 1, not mapRows), so it overlaps the map's own last
    row rather than sitting flush below it — intentional, not an off-by-one.
    */
    {
        FrameBuilder fb;
        fb.markRow(0, 0, totalW);
        fb.markRow(totalH - CELL_SIZE, 0, totalW);
        fb.markCol(0, 0, totalH);
        fb.markCol(totalW - CELL_SIZE, 0, totalH);
        fb.markCol(leftDividerX, 0, totalH);
        fb.markCol(mapRightEdgeX, 0, totalH);
        fb.markRow((mapRows - 1) * CELL_SIZE, mapOriginX, mapRightEdgeX);
        fb.draw();
    }

    /*
    Info box text
    Falls back to MAX_HEIGHT (the map's normal height) when nothing is
    loaded, purely so the info box still sits in its usual place rather
    than jumping around. Location/stats/zoom live on the left panel (see
    above) — the info box just shows the last interaction message.
    */
    const int boxStartY = mapRows + 1; // one row below the map/info-box divider drawn above

    std::vector<std::string> infoLines = GamePanel::buildInfoBoxLines(gGameMessage);

    drawPanelLines(infoLines, mapOriginX, boxStartY);
}

/*
Steps an Objects-layer anchor through frames[1..], forcing an actual
beginFrame/render/endFrame + short delay between each step. Necessary
because App::run() only redraws in response to SDL events (SDL_WaitEvent,
no fixed-rate loop — see core/app.cpp) — without pushing frames out here
ourselves mid-handler, the intermediate animation frames would never hit
the screen at all, just the final state once the event handler returns.
*/
void GameMode::playAnimation(int ax, int ay, const std::vector<TileID>& frames) {
    constexpr int kFrameDelayMs = 120;
    for (size_t i = 1; i < frames.size(); i++) {
        replaceFootprint(ax, ay, frames[i - 1], frames[i]);

        beginFrame();
        render();
        endFrame();

        SDL_Delay(kFrameDelayMs);
    }
}

/*
IMode
onEnter() covers both the startup path (initLevels() + initMap() called
once before the loop) and the path taken when returning from the editor
(`/mode game`) — both collapse to the same "(re)entering game mode"
moment, since switching to and from the editor is just a mode switch
rather than a nested blocking call.

Entering GameMode always (re)loads world/ from disk and starts at
(floor=0, x=0, y=0) — not whatever gCurrentCoord was left at by a previous
session (e.g. after /load'ing elsewhere and then round-tripping through
the editor). If (0,0,0) has no saved file, there's no fallback content:
the game area is simply empty (see render()'s `if (!gCurrentLevel)` guard)
until something is saved there.
*/
void GameMode::onEnter() {
    initLevels();
    if (!enterLevel(0, 0, 0, LevelEntry{})) {
        gCurrentLevel = nullptr;
        gLevelName    = "";
    }
}

/*
Loads the world location at (floor, x, y) and switches the player there,
entering at its default/entityMap-authored spawn point. Used by the
console's /load command (see core/app.cpp) to teleport straight to any
saved location. Returns false (leaving the current location untouched)
if nothing is loaded at that coordinate.
*/
bool GameMode::travelTo(int floor, int x, int y) {
    return enterLevel(floor, x, y, LevelEntry{});
}

void GameMode::onRender() {
    render();
}

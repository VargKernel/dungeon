#ifndef TILES_H
#define TILES_H

#include <cstdint>
#include <vector>

/*
Base tile types
TileID directly encodes X and Y coordinates on the spritesheet.
Y is in the high byte, X is in the low byte.
*/
using TileID = uint16_t;
constexpr TileID EMPTY_ID = 0xFFFF; // Sentinel for empty/null tile

constexpr TileID makeTile(uint8_t x, uint8_t y) { return (y << 8) | x; }
inline uint8_t tileX(TileID id) { return id & 0xFF; }
inline uint8_t tileY(TileID id) { return (id >> 8) & 0xFF; }

// Sentinel stored in the collision layer to mark a cell as blocked.
// Never rendered as a tile sprite; drawn as a yellow outline in the editor.
constexpr TileID COLLISION_MARKER = 0xFFFE;

/*
Sentinels stored in the collision layer to mark floor-transition cells.
Same idea as COLLISION_MARKER — a logical marker, not a spritesheet tile.
The decorative stair sprites (e.g. makeTile(1,18)/(2,18) "going up",
makeTile(1,19)/(2,19) "going down") are placed on the Ground/Objects
layer purely for visuals; these markers are what GameMode actually reads
to trigger a floor change. Drawn as a cyan/magenta outline in the editor.
*/
constexpr TileID STAIRS_UP_MARKER   = 0xFFFD;
constexpr TileID STAIRS_DOWN_MARKER = 0xFFFC;

/*
Sentinel stored in the (separate, invisible) occlusion layer. Marks a
cell where any Objects-layer sprite standing there should be redrawn on
top of the Entities layer — e.g. a door's frame/archway, so the player
reads as walking UNDER it rather than in front of it. Placed one cell at
a time in the editor, exactly like COLLISION_MARKER/STAIRS_*_MARKER (see
editor/editor_controls.cpp's placeTile) — never rendered as a sprite
itself, just a translucent outline in the editor. GameMode reads it in
game_mode.cpp's render().
*/
constexpr TileID OCCLUSION_MARKER = 0xFFFB;

/*
Interactive Logic Tiles
ONLY tiles that have hardcoded behavior in main.cpp are defined here.
All decorative tiles (floors, water, rocks, walls) are entirely handled
by the visual spritesheet and editor. You do NOT need to define them.
*/

constexpr TileID PLAYER      = makeTile(5, 33);

/*
Drawn each frame at the map cell the player is facing (never stored in
any layer's map) — a static overlay sprite marking what E/Space would
act on, no rotation or per-direction variants.
*/
constexpr TileID FACING_INDICATOR = makeTile(4, 33);

// UI Border elements
constexpr TileID HORIZONTAL_BORDER = makeTile(1, 33);
constexpr TileID VERTICAL_BORDER   = makeTile(2, 33);
constexpr TileID CORNER_BORDER     = makeTile(3, 33);

// Layer types
enum class LayerType : uint8_t {
    Ground,    // floors, walls, base room geometry
    Objects,   // furniture, decorations, static objects
    Entities,  // player, NPCs, monsters
    Collision  // logical blocking only — no tile sprite, yellow outline in editor
};

/*
Tile metadata
Describes which layer a tile belongs to and its footprint in cells.
For multitile sprites (w>1 or h>1), the spritesheet is assumed to have
consecutive sub-tiles: columns for X, rows for Y from the anchor position.
*/
struct TileMetadata {
    LayerType layer;
    uint8_t   w;  // width  in cells (1 = single tile)
    uint8_t   h;  // height in cells (1 = single tile)

    /*
    false for tiles that should never show up as a clickable option in
    the editor's palette — UI chrome (borders), the facing-cursor overlay,
    and the logical collision/stairs sentinels. Defaults to true, so
    every ordinary tile registered below is paintable without having to
    say so explicitly; only the handful of special-purpose entries near
    the top of tiles.cpp's registry opt out.
    */
    bool paletteVisible = true;
};

// Returns metadata for a tile. Tiles not in the registry default to {Ground, 1, 1}.
// Register additional tiles in tiles.cpp to enable auto-layer detection and multitile.
TileMetadata getTileMeta(TileID id);

/*
Every tile registered in tiles.cpp's metadata table with paletteVisible
left at true, in a stable top-to-bottom/left-to-right (y, then x) order.
This is the editor's ENTIRE source for what appears in the tile palette —
there is no automatic detection by scanning the spritesheet PNG for
non-transparent pixels any more. To make a new tile paintable, register
it (or update an existing entry) in tiles.cpp; nothing in editor_mode.cpp
needs to change.
*/
std::vector<TileID> getPaletteTiles();

#endif // TILES_H

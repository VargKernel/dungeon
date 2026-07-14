#include "tiles.h"

#include <algorithm>
#include <unordered_map>

// Tile metadata registry
static const std::unordered_map<TileID, TileMetadata> s_meta = {
    // Entities & UI
    { makeTile(5, 33),  { LayerType::Entities, 1, 1 } }, // PLAYER — kept paintable so a spawn point can be placed from the palette
    { makeTile(4, 33),  { LayerType::Entities, 1, 1, false } }, // FACING_INDICATOR — drawn as an overlay only, never placed on a layer

    // Minecart
    { makeTile(6, 18),  { LayerType::Entities, 1, 1 } }, // Full
    { makeTile(6, 19),  { LayerType::Entities, 1, 1 } }, // Empty

    { makeTile(1, 33),  { LayerType::Objects,  1, 1, false } }, // HORIZONTAL_BORDER — drawn automatically by FrameBuilder
    { makeTile(2, 33),  { LayerType::Objects,  1, 1, false } }, // VERTICAL_BORDER
    { makeTile(3, 33),  { LayerType::Objects,  1, 1, false } }, // CORNER_BORDER

    { COLLISION_MARKER,   { LayerType::Collision,1, 1, false } }, // placed via the editor's TAB tool, not the palette
    { STAIRS_UP_MARKER,   { LayerType::Collision,1, 1, false } }, // placed via the editor's [5] tool
    { STAIRS_DOWN_MARKER, { LayerType::Collision,1, 1, false } }, // placed via the editor's [6] tool

    // Floor tiles
    // 1x1
    { makeTile(2, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(3, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(4, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(5, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(6, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(7, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(8, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(9, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(10, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(11, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(12, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(13, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(14, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(15, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(16, 1), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(17, 1), { LayerType::Ground, 1, 1 } },

    // 3x3
    { makeTile(1, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(3, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(1, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(3, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(1, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(3, 4), { LayerType::Ground, 1, 1 } },

    // 3x3
    { makeTile(4, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 4), { LayerType::Ground, 1, 1 } },

    // 3x3
    { makeTile(7, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(7, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(7, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 4), { LayerType::Ground, 1, 1 } },

    // 3x3
    { makeTile(10, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(10, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(10, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 4), { LayerType::Ground, 1, 1 } },

    // 3x3
    { makeTile(13, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 2), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 3), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 4), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 4), { LayerType::Ground, 1, 1 } },

    // 2x2
    { makeTile(8, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 15), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 15), { LayerType::Ground, 1, 1 } },

    // 2x2
    { makeTile(10, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(10, 15), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 15), { LayerType::Ground, 1, 1 } },

    // 2x2
    { makeTile(12, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 15), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 15), { LayerType::Ground, 1, 1 } },

    // 2x2
    { makeTile(14, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 15), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 15), { LayerType::Ground, 1, 1 } },

    // 2x2
    { makeTile(16, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(17, 14), { LayerType::Ground, 1, 1 } },
    { makeTile(16, 15), { LayerType::Ground, 1, 1 } },
    { makeTile(17, 15), { LayerType::Ground, 1, 1 } },

    // Wall tiles
    // 3x3
    { makeTile(1, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(3, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(1, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 6), { LayerType::Ground, 1, 1 } },
    // { makeTile(3, 6), { LayerType::Ground, 1, 1 } }, // Duplicate
    { makeTile(1, 7), { LayerType::Ground, 1, 1 } },
    // { makeTile(2, 7), { LayerType::Ground, 1, 1 } }, // Duplicate
    { makeTile(3, 7), { LayerType::Ground, 1, 1 } },
    // 3x3
    { makeTile(4, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(5, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(6, 7), { LayerType::Ground, 1, 1 } },
    // 3x3
    { makeTile(7, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(7, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(7, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(8, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(9, 7), { LayerType::Ground, 1, 1 } },
    // 3x3
    { makeTile(10, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(10, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 6), { LayerType::Ground, 1, 1 } },
    // { makeTile(12, 6), { LayerType::Ground, 1, 1 } }, // Duplicate
    { makeTile(10, 7), { LayerType::Ground, 1, 1 } },
    // { makeTile(11, 7), { LayerType::Ground, 1, 1 } }, // Duplicate
    { makeTile(12, 7), { LayerType::Ground, 1, 1 } },
    // 3x3
    { makeTile(13, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(14, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(15, 7), { LayerType::Ground, 1, 1 } },
    // 3x3
    { makeTile(16, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(17, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(18, 5), { LayerType::Ground, 1, 1 } },
    { makeTile(16, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(17, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(18, 6), { LayerType::Ground, 1, 1 } },
    { makeTile(16, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(17, 7), { LayerType::Ground, 1, 1 } },
    { makeTile(18, 7), { LayerType::Ground, 1, 1 } },

    // 1x2 - Wall
    { makeTile(1, 8), { LayerType::Ground, 1, 2 } },
    // 1x2 - Door frame
    { makeTile(2, 8), { LayerType::Ground, 1, 2 } },

    // 1x2 - Wall
    { makeTile(10, 8), { LayerType::Ground, 1, 2 } },
    // 1x2 - Door frame
    { makeTile(11, 8), { LayerType::Ground, 1, 2 } },

    // 2x2
    { makeTile(1, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(1, 11), { LayerType::Ground, 1, 1 } },
    { makeTile(2, 11), { LayerType::Ground, 1, 1 } },
    // 2x2
    { makeTile(3, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(3, 11), { LayerType::Ground, 1, 1 } },
    { makeTile(4, 11), { LayerType::Ground, 1, 1 } },
    // 2x2
    { makeTile(10, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(10, 11), { LayerType::Ground, 1, 1 } },
    { makeTile(11, 11), { LayerType::Ground, 1, 1 } },
    // 2x2
    { makeTile(12, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 10), { LayerType::Ground, 1, 1 } },
    { makeTile(12, 11), { LayerType::Ground, 1, 1 } },
    { makeTile(13, 11), { LayerType::Ground, 1, 1 } },

    // 1x1
    { makeTile(1, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(2, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(3, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(4, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(5, 12), { LayerType::Ground, 1, 1 } },

    // 1x1
    { makeTile(10, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(11, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(12, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(13, 12), { LayerType::Ground, 1, 1 } },
    // 1x1
    { makeTile(14, 12), { LayerType::Ground, 1, 1 } },

    // Rocks 1x1
    { makeTile(1, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(2, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(3, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(4, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(5, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(6, 13), { LayerType::Objects, 1, 1 } },
    { makeTile(7, 13), { LayerType::Objects, 1, 1 } },

    // Rocks 1x2
    { makeTile(1, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(2, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(3, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(4, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(5, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(6, 14), { LayerType::Objects, 1, 2 } },
    { makeTile(7, 14), { LayerType::Objects, 1, 2 } },

    // Rails 1x1
    { makeTile(1, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(2, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(3, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(4, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(5, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(6, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(7, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(8, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(9, 16),  { LayerType::Ground, 1, 1 } },
    { makeTile(10, 16), { LayerType::Ground, 1, 1 } },

    // Chests 1х1 (4 sprites for animation)
    { makeTile(13, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(13, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(13, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(13, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(14, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(14, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(14, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(14, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(15, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(15, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(15, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(15, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(16, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(16, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(16, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(16, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(17, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(17, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(17, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(17, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(18, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(18, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(18, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(18, 19), { LayerType::Objects, 1, 1 } },

    { makeTile(19, 16), { LayerType::Objects, 1, 1 } },
    { makeTile(19, 17), { LayerType::Objects, 1, 1 } },
    { makeTile(19, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(19, 19), { LayerType::Objects, 1, 1 } },

    // Furniture 1x1
    // Sign
    { makeTile(1, 17),  { LayerType::Objects, 1, 1 } },

    // Altar
    { makeTile(2, 17),  { LayerType::Objects, 1, 1 } },

    // Stool
    { makeTile(3, 17),  { LayerType::Objects, 1, 1 } },

    // Chair (4 sides)
    { makeTile(4, 17),  { LayerType::Objects, 1, 1 } }, // Right
    { makeTile(5, 17),  { LayerType::Objects, 1, 1 } }, // Down
    { makeTile(6, 17),  { LayerType::Objects, 1, 1 } }, // Up
    { makeTile(7, 17),  { LayerType::Objects, 1, 1 } }, // Left

    // Barrels
    { makeTile(8, 17),  { LayerType::Objects, 1, 1 } }, // Closed
    { makeTile(9, 17),  { LayerType::Objects, 1, 1 } }, // Opened
    { makeTile(10, 17), { LayerType::Objects, 1, 1 } }, // Broken

    // Stairs (Going up)
    { makeTile(1, 18),  { LayerType::Objects, 1, 1 } }, // Left
    { makeTile(2, 18),  { LayerType::Objects, 1, 1 } }, // Right

    // Stairs (Going down)
    { makeTile(1, 19),  { LayerType::Objects, 1, 1 } }, // Left
    { makeTile(2, 19),  { LayerType::Objects, 1, 1 } }, // Right

    // Cobblestone stairs (Going up)
    { makeTile(3, 18),  { LayerType::Objects, 1, 1 } }, // Left
    { makeTile(4, 18),  { LayerType::Objects, 1, 1 } }, // Right

    // Cobblestone stairs (Going down)
    { makeTile(3, 19),  { LayerType::Objects, 1, 1 } }, // Left
    { makeTile(4, 19),  { LayerType::Objects, 1, 1 } }, // Right

    // Paintings 1x1
    { makeTile(5, 24),  { LayerType::Objects, 1, 1 } }, // Variation 1
    { makeTile(6, 24),  { LayerType::Objects, 1, 1 } }, // Variation 2
    { makeTile(5, 25),  { LayerType::Objects, 1, 1 } }, // Variation 3
    { makeTile(6, 25),  { LayerType::Objects, 1, 1 } }, // Variation 4

    // Trap
    { makeTile(7, 18),  { LayerType::Objects, 1, 1 } }, // Deactivated
    { makeTile(7, 19),  { LayerType::Objects, 1, 1 } }, // Activated

    // Cabinet
    { makeTile(8, 18),  { LayerType::Objects, 1, 1 } }, // Clsoed
    { makeTile(8, 19),  { LayerType::Objects, 1, 1 } }, // Opened

    // Wall cabinet
    { makeTile(9, 18),  { LayerType::Objects, 1, 1 } }, // Closed
    { makeTile(9, 19),  { LayerType::Objects, 1, 1 } }, // Opened

    // Large vase
    { makeTile(10, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(10, 19), { LayerType::Objects, 1, 1 } }, // Broken

    // Vase
    { makeTile(11, 18), { LayerType::Objects, 1, 1 } },
    { makeTile(11, 19), { LayerType::Objects, 1, 1 } }, // Broken

    // Furniture 1x2
    // Furnace 1x2
    { makeTile(12, 18), { LayerType::Objects, 1, 2 } },

    // Bookcase 1x2
    { makeTile(1, 20),  { LayerType::Objects, 1, 2 } }, // Empty
    { makeTile(2, 20),  { LayerType::Objects, 1, 2 } }, // With books

    // Wardrobe 1x2
    { makeTile(3, 20),  { LayerType::Objects, 1, 2 } }, // Closed
    { makeTile(4, 20),  { LayerType::Objects, 1, 2 } }, // Opened

    // Statues 1x2
    { makeTile(5, 20),  { LayerType::Objects, 1, 2 } }, // With eyes
    { makeTile(6, 20),  { LayerType::Objects, 1, 2 } }, // Without eyes
    { makeTile(7, 21),  { LayerType::Objects, 1, 1 } }, // Damaged statue - half of sprite

    // Boxes with weapons 1x2
    { makeTile(8, 20),  { LayerType::Objects, 1, 2 } }, // With swords
    { makeTile(9, 20),  { LayerType::Objects, 1, 2 } }, // With spears
    { makeTile(10, 21), { LayerType::Objects, 1, 2 } }, // Empty - half of sprite

    // Banners
    // Simple banners
    { makeTile(11, 20), { LayerType::Objects, 1, 1 } }, // Short
    { makeTile(12, 20), { LayerType::Objects, 1, 2 } }, // Long
    { makeTile(13, 20), { LayerType::Objects, 1, 1 } }, // Damaged hard
    { makeTile(14, 20), { LayerType::Objects, 1, 1 } }, // Damaged medium
    { makeTile(15, 20), { LayerType::Objects, 1, 2 } }, // Damaged low

    // Ornamental banners
    { makeTile(11, 22), { LayerType::Objects, 1, 1 } }, // Short
    { makeTile(12, 22), { LayerType::Objects, 1, 2 } }, // Long
    { makeTile(13, 22), { LayerType::Objects, 1, 1 } }, // Damaged hard
    { makeTile(14, 22), { LayerType::Objects, 1, 1 } }, // Damaged medium
    { makeTile(15, 22), { LayerType::Objects, 1, 2 } }, // Damaged low

    // Furniture 2x2
    // Bookcase 2x2
    { makeTile(1, 22),  { LayerType::Objects, 2, 2 } }, // Empty
    { makeTile(3, 22),  { LayerType::Objects, 2, 2 } }, // With books (Variation 1)
    { makeTile(5, 22),  { LayerType::Objects, 2, 2 } }, // With books (Variation 2)

    // Wall benches 2x2
    { makeTile(7, 22),  { LayerType::Objects, 2, 2 } }, // Variation 1
    { makeTile(9, 22),  { LayerType::Objects, 2, 2 } }, // Variation 2

    // Coffin 2x2
    { makeTile(5, 27),  { LayerType::Objects, 2, 2 } }, // Horizontal opened
    { makeTile(8, 24),  { LayerType::Objects, 2, 2 } }, // Vertical opened

    // Furniture 2x1
    // Paintings 2x1
    { makeTile(1, 24),  { LayerType::Objects, 2, 1 } }, // Variation 1
    { makeTile(3, 24),  { LayerType::Objects, 2, 1 } }, // Variation 2
    { makeTile(1, 25),  { LayerType::Objects, 2, 1 } }, // Variation 3
    { makeTile(3, 25),  { LayerType::Objects, 2, 1 } }, // Variation 4

    // Table 2x1
    { makeTile(11, 16), { LayerType::Objects, 2, 1 } },

    // Bench 2x1
    { makeTile(11, 17), { LayerType::Objects, 2, 1 } },

    // Coffins
    // Coffin 2x1
    { makeTile(5, 26),  { LayerType::Objects, 2, 1 } }, // Horizontal closed
    // Coffin 1x2
    { makeTile(7, 24),  { LayerType::Objects, 1, 2 } }, // Vertical closed

    // Furniture 1x3
    // Column 1x3
    { makeTile(1, 26), { LayerType::Objects, 1, 3 } },

    // Shrine 1x3
    { makeTile(2, 26), { LayerType::Objects, 1, 3 } }, // Default
    { makeTile(3, 26), { LayerType::Objects, 1, 3 } }, // With Spark

    // Skeleton in wall 1x3
    { makeTile(4, 26), { LayerType::Objects, 1, 3 } },

    // Furniture 2x4
    // Large slab
    { makeTile(1, 29), { LayerType::Objects, 2, 4 } },

    // Bookcase 2x4
    { makeTile(3, 29), { LayerType::Objects, 2, 4 } }, // Variation 1
    { makeTile(5, 29), { LayerType::Objects, 2, 4 } }, // Variation 2
    { makeTile(7, 29), { LayerType::Objects, 2, 4 } }, // Variation 3

    // Bookcase 1x4
    { makeTile(9, 29), { LayerType::Objects, 1, 4 } }, // Left
    { makeTile(10, 29), { LayerType::Objects, 1, 4 } }, // Right

    // Doors 1х2 (4 sprites for animation)
    { makeTile(16, 20), { LayerType::Objects, 1, 2 } },
    { makeTile(16, 22), { LayerType::Objects, 1, 2 } },
    { makeTile(16, 24), { LayerType::Objects, 1, 2 } },
    { makeTile(16, 26), { LayerType::Objects, 1, 2 } },

    { makeTile(17, 20), { LayerType::Objects, 1, 2 } },
    { makeTile(17, 22), { LayerType::Objects, 1, 2 } },
    { makeTile(17, 24), { LayerType::Objects, 1, 2 } },
    { makeTile(17, 26), { LayerType::Objects, 1, 2 } },

    { makeTile(18, 20), { LayerType::Objects, 1, 2 } },
    { makeTile(18, 22), { LayerType::Objects, 1, 2 } },
    { makeTile(18, 24), { LayerType::Objects, 1, 2 } },
    { makeTile(18, 26), { LayerType::Objects, 1, 2 } },

    // Nature
    // 4x7
    { makeTile(1, 37), { LayerType::Objects, 4, 7 } }, // Double birch tree

    // 3x7
    { makeTile(5, 37), { LayerType::Objects, 3, 7 } }, // Birch tree large

    // 2x5
    { makeTile(8, 39), { LayerType::Objects, 2, 5 } }, // Birch tree small

    // 3x6
    { makeTile(10, 38), { LayerType::Objects, 3, 6 } }, // Dead tree

    // 2x2
    { makeTile(13, 42), { LayerType::Objects, 2, 2 } }, // Bush

    // Tree cluster

    // Top parts
    // 1x1
    { makeTile(4, 44), { LayerType::Objects, 1, 1 } }, // Right top corner
    // 1x1
    { makeTile(6, 44), { LayerType::Objects, 1, 1 } }, // Left top corner
    // 3x4
    { makeTile(4, 46), { LayerType::Objects, 3, 4 } }, // Top part

    // Middle parts
    // 2x4
    { makeTile(1, 51), { LayerType::Objects, 2, 4 } }, // Left-Middle part

    // 3x4
    { makeTile(4, 51), { LayerType::Objects, 3, 4 } }, // Middle part

    // 2x4
    { makeTile(8, 51), { LayerType::Objects, 2, 4 } }, // Right-Middle part

    // Bottom parts
    // 2x6
    { makeTile(1, 56), { LayerType::Objects, 2, 6 } }, // Left-Bottom part

    // 3x6
    { makeTile(4, 56), { LayerType::Objects, 3, 6 } }, // Bottom part

    // 2x6
    { makeTile(8, 56), { LayerType::Objects, 2, 6 } }, // Right-Bottom part
};

TileMetadata getTileMeta(TileID id) {
    auto it = s_meta.find(id);
    if (it != s_meta.end()) return it->second;

    // Fallback for unknown tiles
    return { LayerType::Ground, 1, 1 };
}

std::vector<TileID> getPaletteTiles() {
    std::vector<TileID> ids;
    ids.reserve(s_meta.size());
    for (const auto& [id, meta] : s_meta)
        if (meta.paletteVisible) ids.push_back(id);

    /*
    s_meta is an unordered_map, so its iteration order isn't something to
    rely on (and isn't guaranteed stable across runs/platforms). Sorting
    by (y, then x) gives the palette a deterministic, sensible order —
    reading the spritesheet top-to-bottom, left-to-right — every time.
    */
    std::sort(ids.begin(), ids.end(), [](TileID a, TileID b) {
        if (tileY(a) != tileY(b)) return tileY(a) < tileY(b);
        return tileX(a) < tileX(b);
    });
    return ids;
}

#include "interactions.h"

#include <unordered_map>

/*
Interactive objects
Only objects listed here respond to anything at all — everything else on
the Objects layer (walls, floor clutter, weapon crates, ...) is purely
decorative as far as GameMode is concerned. Weapon crates are deliberately
NOT registered, so they have no interaction at all.

Interact (E) and Attack (Space) are two SEPARATE tables, each keyed by the
object's current tile — not one shared table filtered by a control field —
because the barrel needs to answer both from the same state (E opens/
closes it, Space breaks it outright) and a single TileID -> Interaction
map can only ever hold one entry per key.

One entry per *current* tile within a table, since e.g. a chest's closed
and open tiles are two different TileIDs and therefore two different rows
(closed answers with the opening animation, open answers with the closing
one).
*/

/*
Interact (E) table
Containers/furniture you open: chests, cabinets, wardrobe, doors, and the
barrel's lid (breaking it outright is Attack's job — see below).
*/
static const std::unordered_map<TileID, Interaction>& interactTable() {
    static const std::unordered_map<TileID, Interaction> t = {
        /*
        Chests — 7 colour variants, 4-frame open animation (row 16 -> 19).
        Closed opens forward through all 4 frames; open closes back
        through them in reverse.
        */
        { makeTile(13, 16), { { makeTile(13,16), makeTile(13,17), makeTile(13,18), makeTile(13,19) }, " You open the chest." } },
        { makeTile(13, 19), { { makeTile(13,19), makeTile(13,18), makeTile(13,17), makeTile(13,16) }, " You close the chest." } },
        { makeTile(14, 16), { { makeTile(14,16), makeTile(14,17), makeTile(14,18), makeTile(14,19) }, " You open the chest." } },
        { makeTile(14, 19), { { makeTile(14,19), makeTile(14,18), makeTile(14,17), makeTile(14,16) }, " You close the chest." } },
        { makeTile(15, 16), { { makeTile(15,16), makeTile(15,17), makeTile(15,18), makeTile(15,19) }, " You open the chest." } },
        { makeTile(15, 19), { { makeTile(15,19), makeTile(15,18), makeTile(15,17), makeTile(15,16) }, " You close the chest." } },
        { makeTile(16, 16), { { makeTile(16,16), makeTile(16,17), makeTile(16,18), makeTile(16,19) }, " You open the chest." } },
        { makeTile(16, 19), { { makeTile(16,19), makeTile(16,18), makeTile(16,17), makeTile(16,16) }, " You close the chest." } },
        { makeTile(17, 16), { { makeTile(17,16), makeTile(17,17), makeTile(17,18), makeTile(17,19) }, " You open the chest." } },
        { makeTile(17, 19), { { makeTile(17,19), makeTile(17,18), makeTile(17,17), makeTile(17,16) }, " You close the chest." } },
        { makeTile(18, 16), { { makeTile(18,16), makeTile(18,17), makeTile(18,18), makeTile(18,19) }, " You open the chest." } },
        { makeTile(18, 19), { { makeTile(18,19), makeTile(18,18), makeTile(18,17), makeTile(18,16) }, " You close the chest." } },
        { makeTile(19, 16), { { makeTile(19,16), makeTile(19,17), makeTile(19,18), makeTile(19,19) }, " You open the chest." } },
        { makeTile(19, 19), { { makeTile(19,19), makeTile(19,18), makeTile(19,17), makeTile(19,16) }, " You close the chest." } },

        // Cabinet / wall cabinet — single closed/open frame each.
        { makeTile(8, 18),  { { makeTile(8,18),  makeTile(8,19)  }, " You open the cabinet." } },
        { makeTile(8, 19),  { { makeTile(8,19),  makeTile(8,18)  }, " You close the cabinet." } },
        { makeTile(9, 18),  { { makeTile(9,18),  makeTile(9,19)  }, " You open the cabinet." } },
        { makeTile(9, 19),  { { makeTile(9,19),  makeTile(9,18)  }, " You close the cabinet." } },

        // Wardrobe — 1x2, single closed/open frame.
        { makeTile(3, 20),  { { makeTile(3,20),  makeTile(4,20)  }, " You open the wardrobe." } },
        { makeTile(4, 20),  { { makeTile(4,20),  makeTile(3,20)  }, " You close the wardrobe." } },

        // Barrel lid — closed <-> opened, freely reversible (unlike the
        // Attack table's break below, this never reaches the broken tile).
        { makeTile(8, 17),  { { makeTile(8,17),  makeTile(9,17)  }, " You open the barrel." } },
        { makeTile(9, 17),  { { makeTile(9,17),  makeTile(8,17)  }, " You close the barrel." } },

        /*
        Doors — 3 variants (columns 16/17/18), each a 1x2 sprite with a
        4-frame open animation (rows 20 -> 22 -> 24 -> 26, same layout as
        the chests' 4-frame rows). Closed opens forward through all 4
        frames; open closes back through them in reverse. Blocking itself
        isn't handled here — see isDoorBlocking() below, which movement
        collision consults directly by tile range instead of a table
        lookup, so a door never needs a COLLISION_MARKER painted under it.
        */
        { makeTile(16, 20), { { makeTile(16,20), makeTile(16,22), makeTile(16,24), makeTile(16,26) }, " You open the door." } },
        { makeTile(16, 26), { { makeTile(16,26), makeTile(16,24), makeTile(16,22), makeTile(16,20) }, " You close the door." } },
        { makeTile(17, 20), { { makeTile(17,20), makeTile(17,22), makeTile(17,24), makeTile(17,26) }, " You open the door." } },
        { makeTile(17, 26), { { makeTile(17,26), makeTile(17,24), makeTile(17,22), makeTile(17,20) }, " You close the door." } },
        { makeTile(18, 20), { { makeTile(18,20), makeTile(18,22), makeTile(18,24), makeTile(18,26) }, " You open the door." } },
        { makeTile(18, 26), { { makeTile(18,26), makeTile(18,24), makeTile(18,22), makeTile(18,20) }, " You close the door." } },
    };
    return t;
}

/*
Attack (Space) table
Things you break: the barrel (from either lid state — closed or already
opened, one hit reduces it straight to splinters) and vases. Both are
one-way/terminal: the resulting tile has no entry in either table, so a
broken object doesn't respond to anything further.
*/
static const std::unordered_map<TileID, Interaction>& attackTable() {
    static const std::unordered_map<TileID, Interaction> t = {
        /*
        Barrel — smashed from closed plays through the open frame on the
        way to broken for a meatier animation; smashed from already-open
        skips straight to broken.
        */
        { makeTile(8, 17),  { { makeTile(8,17), makeTile(9,17), makeTile(10,17) }, " You smash the barrel to splinters." } },
        { makeTile(9, 17),  { { makeTile(9,17), makeTile(10,17) }, " You smash the barrel to splinters." } },

        // Vases — whole -> broken (terminal, one-way).
        { makeTile(10, 18), { { makeTile(10,18), makeTile(10,19) }, " You shatter the vase." } },
        { makeTile(11, 18), { { makeTile(11,18), makeTile(11,19) }, " You shatter the vase." } },
    };
    return t;
}

const Interaction* findInteraction(TileID obj, Control control) {
    const auto& t = (control == Control::Interact) ? interactTable() : attackTable();
    auto it = t.find(obj);
    if (it == t.end()) return nullptr;
    return &it->second;
}

bool isDoorBlocking(TileID obj) {
    /*
    Doors live at spritesheet columns 16-18, rows 20-26 (see tiles.cpp's
    registry) — closed (20) and the two mid-opening frames (22, 24) block;
    the fully-open frame (26) doesn't. tileY covers odd rows too (21, 23,
    25) since a door's bottom half (the 1x2 footprint's second cell)
    holds a raw sub-tile one row below its anchor, not a registry entry
    of its own — see replaceFootprint()/placeTile()'s makeTile(x, y+dy)
    scheme. Only 26/27 (the open anchor + its sub-tile) fall outside the
    blocking range.
    */
    uint8_t x = tileX(obj);
    uint8_t y = tileY(obj);
    return x >= 16 && x <= 18 && y >= 20 && y <= 25;
}

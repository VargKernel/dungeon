#pragma once

#include <string>

#include "../core/level.h"
#include "../core/tiles.h"

/*
Shared game-mode runtime state
game_mode.cpp owns onEnter()/render()/onRender() (state init + drawing);
game_controls.cpp owns onEvent() (keyboard/movement/interaction input) —
see game_mode.h's GameMode class. Both need to read and mutate the same
player/level/HUD state, so it is declared here as extern globals with
whole-program storage duration, reachable from both translation units.
game_mode.cpp is the ONE place that actually defines (allocates) each of
these; every other file only ever sees the extern declarations below.
*/

// Runtime layer arrays
extern TileID gTileLayer     [MAX_HEIGHT][MAX_WIDTH]; // Layer 1: Ground — floors, walls
extern TileID gObjectLayer   [MAX_HEIGHT][MAX_WIDTH]; // Layer 2: Objects — furniture, decorations
extern TileID gEntityLayer   [MAX_HEIGHT][MAX_WIDTH]; // Layer 3: Entities — player, NPCs
extern TileID gCollisionLayer[MAX_HEIGHT][MAX_WIDTH]; // Layer 4: Collision markers (not rendered)
extern TileID gOcclusionMap  [MAX_HEIGHT][MAX_WIDTH]; // Layer 5: Occlusion markers (not rendered)

// Player state
extern int gPlayerPosX;
extern int gPlayerPosY;

/*
Direction the player last moved (or bumped into a wall trying to). Not
persisted across levels/saves — purely a runtime cursor so E/Space and
the facing-cell overlay know which cell to act on. Defaults to Down so a
fresh spawn still has a sane target before any movement key is pressed.
*/
enum class Facing { Up, Down, Left, Right };
extern Facing gPlayerFacing;

/*
The cell the player is currently facing (their own cell + one step in
gPlayerFacing's direction) — the target for E, Space, and the
FACING_INDICATOR overlay drawn in render().
*/
void facingCell(int& fx, int& fy);

/*
World location
gCurrentCoord identifies which location is loaded into the layers above.
gCurrentLevel points at its entry inside worldLevels — stable across
inserts (see level.h), so it's safe to keep between frames/events.
*/
extern LevelCoord gCurrentCoord;
extern Level*     gCurrentLevel;

/*
Level entry point
Describes where the player should appear in the level being entered.
FromAbove/FromBelow are for stairs (arrived via a floor transition,
appear at the matching stairs marker); Edge* are for walking off the map
edge into a neighboring location on the same floor, in which case `carry`
is the along-the-edge coordinate (x for North/South, y for East/West)
to preserve, so crossing a border doesn't teleport you sideways.
*/
struct LevelEntry {
    enum class Kind { Default, FromAbove, FromBelow, EdgeNorth, EdgeSouth, EdgeEast, EdgeWest };
    Kind kind  = Kind::Default;
    int  carry = 0;
};

/*
Central place that switches the active location: looks it up (creating it
blank first only via the editor, never here — a missing neighbor simply
isn't traversable), loads its layers, and updates the HUD title. Returns
false, leaving the current location untouched, if nothing is loaded at
that coordinate.
*/
bool enterLevel(int floor, int x, int y, const LevelEntry& entry);

// HUD strings
extern std::string gLevelName;
extern std::string gGameMessage;

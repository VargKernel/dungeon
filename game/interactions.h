#pragma once

/*
Object interactions
Data-driven table of which Objects-layer tiles respond to player input, and
to which control. This is the single place that knows "this tile does
something" — game_mode.cpp only asks findInteraction() what (if anything)
a given control does to a given tile; it doesn't hardcode per-object logic
itself. Add a new interactive object by adding one entry to the table(s) in
interactions.cpp — nothing in game_mode.cpp needs to change.
*/

#include <string>
#include <vector>

#include "../core/tiles.h"

/*
Which key triggers an interaction. Bound in game_mode.cpp: E -> Interact,
Space -> Attack. Most objects only answer one of the two, but some (the
barrel: E opens/closes it, Space breaks it outright) answer both — from
the SAME current tile — with different results, which is why Interact and
Attack are looked up in two separate tables in interactions.cpp rather
than one shared TileID -> Interaction map (a single map could only ever
hold one entry per tile, so a tile could never answer both).
*/
enum class Control { Interact, Attack };

/*
Describes what happens when a given control is used on an object currently
showing tile `frames[0]`.

frames is the full visual sequence the object steps through, starting
with its current tile and ending on its result tile — e.g. a chest going
from closed to open steps through all 4 of its animation frames, while a
simple two-state object (a cabinet, say) has just [closed, open]. Always
at least 2 elements. game_mode.cpp plays through frames[1..] one at a
time with a short delay between each, so multi-frame animations are
actually visible instead of snapping straight to the end state.

description is shown as the HUD message once the animation finishes, in
English (the codebase's comments/identifiers are English throughout;
this keeps in-game text consistent with that rather than mixing languages
mid-project).
*/
struct Interaction {
    std::vector<TileID>     frames;
    std::string             description;
};

/*
Looks up what happens when `control` is used on an object whose Objects-
layer anchor tile is currently `obj`. Returns nullptr if `obj` has no
registered interaction for `control` at all (e.g. pressing Space on a
chest, which only answers to Interact) — a tile can still separately have
an entry for the OTHER control (e.g. the barrel).
*/
const Interaction* findInteraction(TileID obj, Control control);

/*
True while `obj` (a raw Objects-layer cell — anchor OR sub-tile, since
doors are 1x2) is a closed or still-opening door. Movement collision uses
this so doors block the player while shut without ever needing a
COLLISION_MARKER painted under them on the map (unlike walls/rocks,
which do): the block/unblock state simply follows whichever door-tile
frame is currently sitting in the Objects layer, the same tile the open/
close animation (see interactions.cpp) is already stepping through.
Returns false once the door reaches its fully-open frame, and false for
anything that isn't a door tile at all.
*/
bool isDoorBlocking(TileID obj);

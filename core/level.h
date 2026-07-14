#ifndef LEVELS_H
#define LEVELS_H

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "tiles.h"

// Grid dimensions
const int MAX_WIDTH  = 49;
const int MAX_HEIGHT = 37;

/*
World directory
Every location is one JSON file in this folder, at the project root (a
sibling of CMakeLists.txt / the built executable's working directory) —
NOT inside levels/ or the build tree. Created on demand the first time
something is saved (see saveLevelToFile).
*/
constexpr const char* WORLD_DIR = "world";

/*
Level coordinate
Identifies a single location: which floor it's on, and its (x, y) position
on that floor's grid of locations. Neighboring locations on the same floor
are the ones at (x±1, y) / (x, y±1) — see game_mode.cpp's edge-crossing
logic. Floors are connected only via stairs markers, not by coordinate.
*/
struct LevelCoord {
    int floor = 0;
    int x     = 0;
    int y     = 0;

    bool operator==(const LevelCoord& o) const {
        return floor == o.floor && x == o.x && y == o.y;
    }
};

struct LevelCoordHash {
    size_t operator()(const LevelCoord& c) const {
        size_t h = std::hash<int>()(c.floor);
        h = h * 1000003u ^ std::hash<int>()(c.x);
        h = h * 1000003u ^ std::hash<int>()(c.y);
        return h;
    }
};

/*
File name for a location's JSON, e.g. "world/LEVEL-00-01-00.json".
Coordinates may be negative — %02d still produces an unambiguous name
(e.g. "LEVEL-00--01-00.json") since parsing (see loadWorld) reads the
three integers with sscanf rather than splitting on '-'.
*/
std::string levelFileName(int floor, int x, int y);

// Level data
struct Level {
    TileID tileMap     [MAX_HEIGHT][MAX_WIDTH];  // Layer 1: Ground — floors, walls, base geometry
    TileID collisionMap[MAX_HEIGHT][MAX_WIDTH];  // Layer 4: Collision markers (COLLISION_MARKER / STAIRS_*_MARKER)
    TileID entityMap   [MAX_HEIGHT][MAX_WIDTH];  // Layer 3: Entities — player, NPCs, monsters
    TileID objectMap   [MAX_HEIGHT][MAX_WIDTH];  // Layer 2: Objects — furniture, decorations, static objects
    TileID occlusionMap[MAX_HEIGHT][MAX_WIDTH];  // Layer 5: Occlusion markers (OCCLUSION_MARKER) — see tiles.h

    int  width  = MAX_WIDTH;
    int  height = MAX_HEIGHT;
    char name[64] = "New Location"; // Display name shown in the UI info box
    bool isStatic = true;           // Reserved: true = hand-crafted, false = procedurally generated

    /*
    Where this location lives in the world. Kept on the Level itself (as
    well as being the key it's stored under in `worldLevels`) so a Level*
    handed to GameMode/EditorMode is self-describing.
    */
    int floor = 0;
    int locX  = 0;
    int locY  = 0;
};

/*
All currently loaded locations, keyed by coordinate. Populated by
initLevels() at startup (and refreshed by the editor after a save).
unordered_map gives stable references/pointers across inserts (no
rehash invalidates an existing element's address) — safe for GameMode to
hold a `Level*` to the current location across frames.
*/
extern std::unordered_map<LevelCoord, Level, LevelCoordHash> worldLevels;

/*
Scans world/*.json, loading every file matching LEVEL-<floor>-<x>-<y>.json
into worldLevels. No fallback content: if world/ is empty, worldLevels
ends up empty too — there is no synthesized placeholder location.
*/
void initLevels();

// Looks up an already-loaded location. Returns nullptr if none is loaded at
// that coordinate (i.e. no such file was found by initLevels/loadWorld).
Level* findLevel(int floor, int x, int y);

/*
Returns the location at (floor, x, y), creating a fresh blank one
in-memory (registered in worldLevels, but NOT written to disk) if it
doesn't already exist. Used by the editor when the selected coordinate
doesn't correspond to a saved file yet.
*/
Level& getOrCreateLevel(int floor, int x, int y);

// All currently loaded coordinates, sorted by (floor, x, y). Used by the
// editor's location-list panel and by console autocomplete for /load.
std::vector<LevelCoord> listWorldLevels();

// Load one level from a JSON file produced by the editor.
// Returns true on success; Level is left unchanged on failure.
bool loadLevelFromFile(Level& level, const char* path);

// Write `level` out as JSON to `path` (creating parent directories as
// needed). Returns true on success.
bool saveLevelToFile(const Level& level, const char* path);

#endif // LEVELS_H

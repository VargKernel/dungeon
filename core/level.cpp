#include "level.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <vector>

/*
Map authoring notes
Locations are stored one-per-file in world/LEVEL-<floor>-<x>-<y>.json
and loaded at startup by initLevels(). In the Editor: F5 saves the map
currently being edited to the selected coordinate, F6 loads the selected
coordinate's file back into the editor. See editor/editor_mode.cpp.
*/

std::unordered_map<LevelCoord, Level, LevelCoordHash> worldLevels;

std::string levelFileName(int floor, int x, int y) {
    char buf[96];
    snprintf(buf, sizeof(buf), "%s/LEVEL-%02d-%02d-%02d.json", WORLD_DIR, floor, x, y);
    return std::string(buf);
}

static void fillEmpty(Level& level) {
    for (int y = 0; y < MAX_HEIGHT; ++y)
        for (int x = 0; x < MAX_WIDTH; ++x) {
            level.tileMap[y][x]      = EMPTY_ID;
            level.collisionMap[y][x] = EMPTY_ID;
            level.entityMap[y][x]    = EMPTY_ID;
            level.objectMap[y][x]    = EMPTY_ID;
            level.occlusionMap[y][x] = EMPTY_ID;
        }
}

/*
Minimal JSON parser
Handles only the level JSON format produced by the editor:
a single object whose values are strings, integers, booleans, or
arrays-of-arrays-of-unsigned-integers.  No external dependencies.
*/

static const char* jSkip(const char* p) {
    if (!p) return p;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
    return p;
}

// Advance past one expected character (skipping leading whitespace).
static const char* jChar(const char* p, char c) {
    p = jSkip(p);
    return (p && *p == c) ? p + 1 : p;
}

// Parse a JSON string literal into out[0..maxLen-1]; returns p past closing ".
static const char* jString(const char* p, char* out, int maxLen) {
    p = jChar(p, '"');
    int i = 0;
    while (p && *p && *p != '"' && i < maxLen - 1) out[i++] = *p++;
    out[i] = '\0';
    if (p && *p == '"') ++p;
    return p;
}

// Parse a JSON integer (optionally negative) into *v.
static const char* jInt(const char* p, long long* v) {
    p = jSkip(p);
    bool neg = (p && *p == '-');
    if (neg) ++p;
    *v = 0;
    while (p && *p >= '0' && *p <= '9') *v = *v * 10 + (*p++ - '0');
    if (neg) *v = -*v;
    return p;
}

// Parse a JSON bool (true / false) into *v.
static const char* jBool(const char* p, bool* v) {
    p = jSkip(p);
    if (p && strncmp(p, "true",  4) == 0) { *v = true;  return p + 4; }
    if (p && strncmp(p, "false", 5) == 0) { *v = false; return p + 5; }
    return p;
}

// Skip one complete JSON value (used to ignore unknown keys).
static const char* jSkipValue(const char* p) {
    p = jSkip(p);
    if (!p || !*p) return p;

    if (*p == '"') {
        // String
        ++p;
        while (*p && *p != '"') { if (*p == '\\') ++p; if (*p) ++p; }
        if (*p == '"') ++p;
    } else if (*p == '[' || *p == '{') {
        // Array or object — track bracket depth
        char open = *p, close = (open == '[') ? ']' : '}';
        ++p;
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '"') {
                ++p;
                while (*p && *p != '"') { if (*p == '\\') ++p; if (*p) ++p; }
                if (*p == '"') ++p;
            } else if (*p == open)  { ++depth; ++p; }
            else if (*p == close) { --depth; ++p; }
            else                  { ++p; }
        }
    } else if (*p == 't') { p += 4; }  // true
    else if (*p == 'f')   { p += 5; }  // false
    else if (*p == 'n')   { p += 4; }  // null
    else {
        // Number: skip all numeric characters
        while (*p && *p != ',' && *p != ']' && *p != '}') ++p;
    }
    return p;
}

/*
Parse a JSON 2D array [[uint,...], ...] into map[MAX_HEIGHT][MAX_WIDTH].
Always reads exactly MAX_HEIGHT rows and MAX_WIDTH columns to match what the
editor writes, regardless of the level's declared width/height.
*/
static const char* j2DArray(const char* p, TileID map[][MAX_WIDTH]) {
    p = jChar(p, '[');
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        p = jChar(p, '[');
        for (int x = 0; x < MAX_WIDTH; ++x) {
            long long v = 0;
            p = jInt(p, &v);
            map[y][x] = (TileID)(unsigned short)v;
            p = jSkip(p);
            if (p && *p == ',') ++p; // skip value separator
        }
        p = jChar(p, ']');
        p = jSkip(p);
        if (p && *p == ',') ++p; // skip row separator
    }
    p = jChar(p, ']');
    return p;
}

// loadLevelFromFile
bool loadLevelFromFile(Level& level, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    std::vector<char> buf(len + 1);
    fread(buf.data(), 1, (size_t)len, f);
    fclose(f);
    buf[len] = '\0';

    const char* p = jChar(buf.data(), '{');

    while (p && *p && *p != '}') {
        char key[64] = {};
        p = jString(p, key, sizeof(key));
        p = jChar(p, ':');

        if      (strcmp(key, "name")         == 0) { p = jString(p, level.name, 64); }
        else if (strcmp(key, "floor")        == 0) { long long v; p = jInt(p, &v); level.floor  = (int)v; }
        else if (strcmp(key, "x")            == 0) { long long v; p = jInt(p, &v); level.locX   = (int)v; }
        else if (strcmp(key, "y")            == 0) { long long v; p = jInt(p, &v); level.locY   = (int)v; }
        else if (strcmp(key, "width")        == 0) { long long v; p = jInt(p, &v); level.width  = (int)v; }
        else if (strcmp(key, "height")       == 0) { long long v; p = jInt(p, &v); level.height = (int)v; }
        else if (strcmp(key, "isStatic")     == 0) { p = jBool(p, &level.isStatic); }
        else if (strcmp(key, "tileMap")      == 0) { p = j2DArray(p, level.tileMap); }
        else if (strcmp(key, "collisionMap") == 0) { p = j2DArray(p, level.collisionMap); }
        else if (strcmp(key, "entityMap")    == 0) { p = j2DArray(p, level.entityMap); }
        else if (strcmp(key, "objectMap")    == 0) { p = j2DArray(p, level.objectMap); }
        else if (strcmp(key, "occlusionMap") == 0) { p = j2DArray(p, level.occlusionMap); }
        else                                        { p = jSkipValue(p); } // ignore unknown keys

        p = jSkip(p);
        if (p && *p == ',') ++p; // skip key-value separator
        p = jSkip(p);
    }

    return true;
}

// JSON writer
static void jWrite2DArray(FILE* f, const TileID map[MAX_HEIGHT][MAX_WIDTH]) {
    fprintf(f, "[\n");
    for (int y = 0; y < MAX_HEIGHT; ++y) {
        fprintf(f, "    [");
        for (int x = 0; x < MAX_WIDTH; ++x)
            fprintf(f, "%u%s", (unsigned)map[y][x], (x + 1 < MAX_WIDTH) ? "," : "");
        fprintf(f, "]%s\n", (y + 1 < MAX_HEIGHT) ? "," : "");
    }
    fprintf(f, "  ]");
}

/*
Escapes '"' and '\\' so a name typed with either doesn't corrupt the JSON.
Not full JSON string escaping (no control-character handling) — the name
field is short, editor-typed text, not arbitrary content.
*/
static std::string jEscape(const char* s) {
    std::string out;
    for (const char* p = s; *p; ++p) {
        if (*p == '"' || *p == '\\') out += '\\';
        out += *p;
    }
    return out;
}

bool saveLevelToFile(const Level& level, const char* path) {
    std::error_code ec;
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path(), ec);

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", jEscape(level.name).c_str());
    fprintf(f, "  \"floor\": %d,\n", level.floor);
    fprintf(f, "  \"x\": %d,\n", level.locX);
    fprintf(f, "  \"y\": %d,\n", level.locY);
    fprintf(f, "  \"width\": %d,\n", level.width);
    fprintf(f, "  \"height\": %d,\n", level.height);
    fprintf(f, "  \"isStatic\": %s,\n", level.isStatic ? "true" : "false");
    fprintf(f, "  \"tileMap\": ");      jWrite2DArray(f, level.tileMap);      fprintf(f, ",\n");
    fprintf(f, "  \"objectMap\": ");    jWrite2DArray(f, level.objectMap);    fprintf(f, ",\n");
    fprintf(f, "  \"entityMap\": ");    jWrite2DArray(f, level.entityMap);    fprintf(f, ",\n");
    fprintf(f, "  \"collisionMap\": "); jWrite2DArray(f, level.collisionMap); fprintf(f, ",\n");
    fprintf(f, "  \"occlusionMap\": "); jWrite2DArray(f, level.occlusionMap); fprintf(f, "\n");
    fprintf(f, "}\n");

    fclose(f);
    return true;
}

// World access
Level* findLevel(int floor, int x, int y) {
    auto it = worldLevels.find(LevelCoord{floor, x, y});
    return (it != worldLevels.end()) ? &it->second : nullptr;
}

Level& getOrCreateLevel(int floor, int x, int y) {
    LevelCoord key{floor, x, y};
    auto it = worldLevels.find(key);
    if (it != worldLevels.end()) return it->second;

    Level level;
    fillEmpty(level);
    level.width  = MAX_WIDTH;
    level.height = MAX_HEIGHT;
    level.isStatic = true;
    snprintf(level.name, sizeof(level.name), "New Location");
    level.floor = floor;
    level.locX  = x;
    level.locY  = y;

    return worldLevels.emplace(key, level).first->second;
}

std::vector<LevelCoord> listWorldLevels() {
    std::vector<LevelCoord> coords;
    coords.reserve(worldLevels.size());
    for (const auto& [key, level] : worldLevels) coords.push_back(key);

    std::sort(coords.begin(), coords.end(), [](const LevelCoord& a, const LevelCoord& b) {
        if (a.floor != b.floor) return a.floor < b.floor;
        if (a.x     != b.x)     return a.x     < b.x;
        return a.y < b.y;
    });
    return coords;
}

/*
initLevels
Called once at startup (before entering GameMode) and again whenever the
editor or /load wants a fresh view of what's on disk. Scans world/ for
every LEVEL-<floor>-<x>-<y>.json file and loads it — nothing more. There
is no fallback placeholder: if world/ is empty (or (0,0,0) specifically
isn't in it), worldLevels ends up empty (or missing that coordinate), and
callers are expected to handle "nothing loaded here" themselves rather
than silently getting synthesized content.
*/
void initLevels() {
    worldLevels.clear();

    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(WORLD_DIR, ec) && fs::is_directory(WORLD_DIR, ec)) {
        for (const auto& entry : fs::directory_iterator(WORLD_DIR, ec)) {
            if (ec || !entry.is_regular_file()) continue;

            std::string fname = entry.path().filename().string();
            int floor = 0, x = 0, y = 0;
            if (sscanf(fname.c_str(), "LEVEL-%d-%d-%d.json", &floor, &x, &y) != 3)
                continue; // not a location file — ignore

            Level level;
            fillEmpty(level);
            level.width  = MAX_WIDTH;
            level.height = MAX_HEIGHT;
            level.isStatic = true;
            snprintf(level.name, sizeof(level.name), "Location %d/%d/%d", floor, x, y);

            if (loadLevelFromFile(level, entry.path().string().c_str())) {
                /*
                The filename is authoritative for where this location
                lives in the world, regardless of what's in the JSON body
                (guards against a hand-edited/renamed file disagreeing
                with its own coordinate fields).
                */
                level.floor = floor;
                level.locX  = x;
                level.locY  = y;
                worldLevels[LevelCoord{floor, x, y}] = level;
            }
        }
    }
}

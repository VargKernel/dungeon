#include "game_mode.h"

#include "../core/level.h"
#include "../core/renderer.h"
#include "../core/tiles.h"
#include "../settings/settings_mode.h"
#include "game_state.h"
#include "interactions.h"

/*
resolveInteractionAnchor
Multi-cell Objects-layer sprites (currently: 1x2 doors, the 1x2 wardrobe)
only register their TOP-LEFT anchor tile in interactions.cpp's table —
every other cell of the footprint holds a raw sub-tile stamped by
replaceFootprint (the anchor's TileID with a (dx,dy) offset baked in, see
game_mode.cpp), which isn't a registered interaction key on its own. So
facing anything but the anchor itself (e.g. the bottom half of a door)
makes a direct findInteraction() lookup fail even though the object very
much has a registered interaction.

This walks backward from (fx, fy) looking for a cell whose tile, offset by
(dx, dy), would produce exactly the raw tile the player is facing — i.e.
"is (fx, fy) cell (dx, dy) inside SOME anchor's footprint", searching
backward from the facing cell rather than forward from a spritesheet scan.
kMaxSpan bounds the search to the largest footprint any registered tile
could plausibly have; every real object today is only 1x2, so this rarely
walks more than one cell.
*/
static bool resolveInteractionAnchor(int fx, int fy, int& ax, int& ay) {
    constexpr int kMaxSpan = 8;
    TileID facingRaw = gObjectLayer[fy][fx];

    for (int dy = 0; dy < kMaxSpan && fy - dy >= 0; ++dy) {
        for (int dx = 0; dx < kMaxSpan && fx - dx >= 0; ++dx) {
            if (dx == 0 && dy == 0) continue;

            int cx = fx - dx, cy = fy - dy;
            TileID candidate = gObjectLayer[cy][cx];
            TileMetadata meta = getTileMeta(candidate);
            if (dx >= meta.w || dy >= meta.h) continue; // candidate's footprint doesn't reach (fx, fy)

            if (facingRaw == makeTile(tileX(candidate) + dx, tileY(candidate) + dy)) {
                ax = cx; ay = cy;
                return true;
            }
        }
    }
    return false;
}

/*
GameMode::onEvent
All keyboard input for game mode: movement, Interact/Attack, zoom, quit.
Defined here rather than in game_mode.cpp — still a member of GameMode,
so it can call playAnimation() just as freely, since member-function
access rules don't depend on which .cpp the definition physically lives
in. The state it reads/writes (player position, layers, current level,
HUD messages) is declared in game_state.h and defined in game_mode.cpp,
since that's also where render()/initMap() need it.
*/
void GameMode::onEvent(const SDL_Event& e) {
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_EXPOSED) {
        return; // App already redraws on every event; nothing extra to do here
    }

    if (e.type != SDL_KEYDOWN) return;

    char input = 0;
    bool moved = false;
    int  nextX = gPlayerPosX;
    int  nextY = gPlayerPosY;

    switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            input = 'w';
            break;

        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            input = 's';
            break;

        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            input = 'a';
            break;

        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            input = 'd';
            break;

        case SDL_SCANCODE_E:
            input = 'e';
            break;

        case SDL_SCANCODE_SPACE:
            input = ' ';
            break;
        /*
        Esc opens the settings screen (see settings_mode.h's class comment
        on setReturnMode); Q is the only thing that quits, consistently
        across every mode. Switching to the editor is a console-only
        action (`/mode editor`); see core/app.cpp's dispatchCommand.
        */
        case SDL_SCANCODE_ESCAPE:
            SettingsMode::setReturnMode(SettingsMode::ReturnMode::Game);
            context_.switchMode(std::make_unique<SettingsMode>());
            return;

        case SDL_SCANCODE_Q:
            context_.requestQuit();
            return;

        case SDL_SCANCODE_EQUALS:
        case SDL_SCANCODE_KP_PLUS:
            setZoom(getZoom() + 1);
            return;

        case SDL_SCANCODE_MINUS:
        case SDL_SCANCODE_KP_MINUS:
            setZoom(getZoom() - 1);
            return;

        default: break;
    }

    if (input == 0) return;
    if (!gCurrentLevel) return;
    Level& level = *gCurrentLevel;

    /*
    W/A/S/D always turns the player to face that direction, whether or not
    the step itself ends up blocked below — bumping into a wall still
    turns you to face it, same as most tile-based games.
    */
    if      (input == 'w') { gPlayerFacing = Facing::Up;    nextY--; moved = true; }
    else if (input == 's') { gPlayerFacing = Facing::Down;  nextY++; moved = true; }
    else if (input == 'a') { gPlayerFacing = Facing::Left;  nextX--; moved = true; }
    else if (input == 'd') { gPlayerFacing = Facing::Right; nextX++; moved = true; }
    else if (input == 'e' || input == ' ') {
        /*
        E (Interact) and Space (Attack) both act on whatever the player
        is facing, not the tile they're standing on (see facingCell()) —
        reads/writes the Objects layer, since that's where furniture/
        decoration actually live. What each control does to what is all
        in game/interactions.h/.cpp; this just looks it up and plays it.
        */
        Control control = (input == 'e') ? Control::Interact : Control::Attack;
        int fx, fy;
        facingCell(fx, fy);
        if (fx >= 0 && fx < level.width && fy >= 0 && fy < level.height) {
            const Interaction* inter = findInteraction(gObjectLayer[fy][fx], control);

            /*
            Direct lookup failed — the facing cell might be a sub-tile of
            a multi-cell object (e.g. the bottom half of a 1x2 door)
            rather than its registered anchor. Resolve back to the
            anchor and retry once before giving up on it entirely.
            */
            if (!inter) {
                int ax = fx, ay = fy;
                if (resolveInteractionAnchor(fx, fy, ax, ay)) {
                    inter = findInteraction(gObjectLayer[ay][ax], control);
                    if (inter) { fx = ax; fy = ay; }
                }
            }

            if (inter) {
                playAnimation(fx, fy, inter->frames);
                gGameMessage = inter->description;
            }
        }
    }

    if (!moved) return;

    /*
    Edge crossing
    Stepping past the current map's own outermost ring of cells hands the
    player to the neighboring location on the same floor. Horizontally,
    that ring sits a full cell beyond the map's actual array bounds
    (nextX < 0 / nextX >= level.width), so the player can walk onto column
    0 and column width - 1 before crossing over. This never touches array
    bounds itself: every branch below returns before nextX is used as an
    index, so nextX == -1 or nextX == level.width are only ever compared,
    never dereferenced. Vertically the ring is exactly one cell in from
    the array bounds (nextY <= 0 / nextY >= height - 1).
    If there isn't a neighboring location loaded, movement is simply
    blocked at the edge (nothing to fall through to below either way,
    since border cells are never walkable, so there's no separate wall
    check needed for them).
    */
    if (nextX < 0)                 { if (enterLevel(gCurrentCoord.floor, gCurrentCoord.x - 1, gCurrentCoord.y, LevelEntry{LevelEntry::Kind::EdgeEast,  gPlayerPosY})) { gGameMessage = ""; } return; }
    if (nextX >= level.width)      { if (enterLevel(gCurrentCoord.floor, gCurrentCoord.x + 1, gCurrentCoord.y, LevelEntry{LevelEntry::Kind::EdgeWest,  gPlayerPosY})) { gGameMessage = ""; } return; }
    if (nextY <= 0)                { if (enterLevel(gCurrentCoord.floor, gCurrentCoord.x, gCurrentCoord.y - 1, LevelEntry{LevelEntry::Kind::EdgeSouth, gPlayerPosX})) { gGameMessage = ""; } return; }
    if (nextY >= level.height - 1) { if (enterLevel(gCurrentCoord.floor, gCurrentCoord.x, gCurrentCoord.y + 1, LevelEntry{LevelEntry::Kind::EdgeNorth, gPlayerPosX})) { gGameMessage = ""; } return; }

    /*
    A cell blocks movement if it carries a collision marker, OR if a
    closed/still-opening door sits on the Objects layer there — doors
    block purely by which tile they currently are (see isDoorBlocking) —
    no COLLISION_MARKER is ever painted under one on the map itself.
    Stairs markers are intentionally NOT blocking — walking onto one
    steps onto the stairs tile itself, and the floor transition below
    then fires immediately.
    */
    bool isWall = (gCollisionLayer[nextY][nextX] == COLLISION_MARKER) ||
                  isDoorBlocking(gObjectLayer[nextY][nextX]);

    if (!isWall) {
        gEntityLayer[gPlayerPosY][gPlayerPosX] = EMPTY_ID;
        gPlayerPosX = nextX;
        gPlayerPosY = nextY;
        gEntityLayer[gPlayerPosY][gPlayerPosX] = PLAYER;
        gGameMessage = "";

        setMapCamera(gPlayerPosX, gPlayerPosY);

        /*
        Stairs only trigger a floor change here — inside the "the move
        actually happened" branch — rather than unconditionally on the
        player's current cell every keypress. That's what stops a
        ping-pong loop: a level entered via FromAbove/FromBelow lands the
        player exactly on the matching stairs marker (see initMap()), so
        if this check ran even when the move was blocked (e.g. a
        directional key bumped a wall right after arriving, leaving the
        player still standing on that same marker), it would immediately
        fire again and bounce them straight back to the floor they just
        left. Gating it on a fresh, successful step onto the marker means
        arriving on one is a one-time landing — only actually stepping
        onto it (this frame or a later one) transfers the player again.
        */
        if (gCollisionLayer[gPlayerPosY][gPlayerPosX] == STAIRS_DOWN_MARKER) {
            if (!enterLevel(gCurrentCoord.floor - 1, gCurrentCoord.x, gCurrentCoord.y, LevelEntry{LevelEntry::Kind::FromAbove, 0}))
                gGameMessage = " The way down hasn't been built yet.";
        }
        else if (gCollisionLayer[gPlayerPosY][gPlayerPosX] == STAIRS_UP_MARKER) {
            if (!enterLevel(gCurrentCoord.floor + 1, gCurrentCoord.x, gCurrentCoord.y, LevelEntry{LevelEntry::Kind::FromBelow, 0}))
                gGameMessage = " The way up hasn't been built yet.";
        }
    }
}

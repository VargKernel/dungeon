#include "editor_mode.h"

#include "../core/display.h"
#include "../core/level.h"
#include "../core/renderer.h"
#include "../core/tiles.h"
#include "../settings/settings_mode.h"
#include "editor_state.h"

// placeTile
void placeTile(int gx, int gy) {
    if (activeLayer == EditLayer::COLLISION) {
        edCollisionMap[gy][gx] = (activeCollisionTool == CollisionTool::STAIRS_UP)   ? STAIRS_UP_MARKER
                                : (activeCollisionTool == CollisionTool::STAIRS_DOWN) ? STAIRS_DOWN_MARKER
                                                                                       : COLLISION_MARKER;
        return;
    }

    if (activeLayer == EditLayer::OCCLUSION) {
        /*
        Single-cell, exactly like COLLISION_MARKER/STAIRS_*_MARKER — no
        auto-stamping. A 3-tall doorway occlusion zone is 3 separate
        clicks, same as any other marker (see tiles.h's OCCLUSION_MARKER).
        */
        edOcclusionMap[gy][gx] = OCCLUSION_MARKER;
        return;
    }

    TileMetadata meta = getTileMeta(selectedTile);

    if (gx + meta.w > MAX_WIDTH || gy + meta.h > MAX_HEIGHT) return;

    TileID (*map)[MAX_WIDTH] = getLayerMap(activeLayer);

    for (int dy = 0; dy < meta.h; ++dy) {
        for (int dx = 0; dx < meta.w; ++dx) {
            int cx = gx + dx, cy = gy + dy;
            if (mtMap[cy][cx].occupied) {
                int ax = mtMap[cy][cx].anchorX;
                int ay = mtMap[cy][cx].anchorY;
                TileMetadata old = getTileMeta(map[ay][ax]);
                for (int oy = 0; oy < old.h && ay + oy < MAX_HEIGHT; ++oy)
                    for (int ox = 0; ox < old.w && ax + ox < MAX_WIDTH; ++ox) {
                        map[ay + oy][ax + ox] = EMPTY_ID;
                        mtMap[ay + oy][ax + ox] = { false, 0, 0 };
                    }
            }
        }
    }

    for (int dy = 0; dy < meta.h; ++dy) {
        for (int dx = 0; dx < meta.w; ++dx) {
            int cx = gx + dx, cy = gy + dy;
            map[cy][cx] = makeTile(tileX(selectedTile) + dx, tileY(selectedTile) + dy);
            if (dx == 0 && dy == 0) {
                mtMap[cy][cx] = { false, 0, 0 };
            } else {
                mtMap[cy][cx] = { true, gx, gy };
            }
        }
    }
}

// eraseTile
void eraseTile(int gx, int gy) {
    if (activeLayer == EditLayer::COLLISION) {
        edCollisionMap[gy][gx] = EMPTY_ID;
        return;
    }

    if (activeLayer == EditLayer::OCCLUSION) {
        edOcclusionMap[gy][gx] = EMPTY_ID;
        return;
    }

    TileID (*map)[MAX_WIDTH] = getLayerMap(activeLayer);

    int ax = gx, ay = gy;
    if (mtMap[gy][gx].occupied) {
        ax = mtMap[gy][gx].anchorX;
        ay = mtMap[gy][gx].anchorY;
    }

    TileMetadata meta = getTileMeta(map[ay][ax]);
    for (int dy = 0; dy < meta.h; ++dy) {
        for (int dx = 0; dx < meta.w; ++dx) {
            int cx = ax + dx, cy = ay + dy;
            if (cx < MAX_WIDTH && cy < MAX_HEIGHT) {
                map[cy][cx] = EMPTY_ID;
                mtMap[cy][cx] = { false, 0, 0 };
            }
        }
    }
}

/*
handlePointerAction
The original re-evaluated this every ~16ms from SDL_GetMouseState() while a
button was held. Here it runs once per relevant SDL event (button-down, or
motion while a button is held) — same observable click/drag behaviour.
*/
static void handlePointerAction(int mx, int my, int panelContentW, int mapOriginX,
                                 int mapRightEdgeX, int rightPanelX) {
    if (mx >= rightPanelX) {
        /*
        Right panel: click a row to select that location as the F5/F6
        target. Header row (coordinate + name) occupies the first cell,
        same PALETTE_MARGIN-relative layout as the palette panel — see
        EditorMode::onRender()'s "Right panel" block for the matching
        draw code (both use EditorPanel::WORLD_LIST_START_ROW).
        */
        if (!mouseDown_) return;
        int rowH       = CELL_SIZE;
        int listStartY = PALETTE_MARGIN + rowH * 2; // header + coord-stepper rows
        int rowIndex   = (my + worldListScroll - listStartY) / rowH;

        std::vector<LevelCoord> coords = listWorldLevels();
        if (rowIndex >= 0 && rowIndex < (int)coords.size())
            selectedCoord = coords[rowIndex];
        return;
    }

    if (mx < panelContentW) {
        /*
        The whole left panel is the palette — no tab bar, no preview box
        carving out a sub-region — so every click here is a tile pick.
        Tiles don't sit on a regular col/row grid (shelf packing gives each
        one its own full-sprite-sized rect), so picking one means testing
        the click point against each tile's rect directly rather than
        dividing by a fixed cell size.
        */
        if (mouseDown_) {
            int adjustedX = mx;
            int adjustedY = my + paletteScroll;

            for (const PaletteEntry& entry : computePaletteLayout(panelContentW)) {
                const SDL_Rect& r = entry.rect;
                if (adjustedX >= r.x && adjustedX < r.x + r.w &&
                    adjustedY >= r.y && adjustedY < r.y + r.h) {
                    selectedTile = entry.id;
                    EditLayer detected = layerForTile(selectedTile);
                    if (detected != EditLayer::COLLISION)
                        activeLayer = detected;
                    break;
                }
            }
        }
    }
    else if (mx >= mapOriginX && mx < mapRightEdgeX) {
        int gx = (mx - mapOriginX) / CELL_SIZE;
        int gy = my / CELL_SIZE;

        /*
        Top and bottom rows (gy == 0 and gy == MAX_HEIGHT - 1) are
        excluded from painting — the editable play area is 16px shorter
        on each vertical edge than the full MAX_HEIGHT grid it's stored
        as. The grid itself is unchanged; this is purely a placement
        restriction (see handlePointerAction's caller for the matching
        horizontal bounds, which are NOT restricted).
        */
        if (gx >= 0 && gx < MAX_WIDTH && gy >= 1 && gy < MAX_HEIGHT - 1) {
            if (rightMouseDown_) eraseTile(gx, gy);
            else                 placeTile(gx, gy);
        }
    }
    // else: click landed in one of the divider columns (panel-to-map or
    // map-to-panel gap) — not interactive.
}

// Converts a real-window mouse position to logical-canvas coordinates and
// dispatches it, ignoring clicks that land in the letterbox border.
static void handlePointerEvent(int windowX, int windowY) {
    int winW = 0, winH = 0;
    SDL_GetWindowSize(getWindow(), &winW, &winH);

    int lx, ly;
    if (!displayWindowToLogical(windowX, windowY, winW, winH, lx, ly))
        return;

    handlePointerAction(lx, ly, PANEL_W, MAP_ORIGIN_X, MAP_RIGHT_EDGE_X, RIGHT_PANEL_X);
}

/*
EditorMode::onEvent
All keyboard/mouse input for editor mode: tool selection, location
stepping, save/load, painting/erasing, and palette/world-list scrolling.
Defined here rather than in editor_mode.cpp — still a member of
EditorMode, so it can call context_.switchMode()/requestQuit() just as
freely, since member-function access rules don't depend on which .cpp the
definition physically lives in. The state it reads/writes (ed*Map, palette
selection, selectedCoord, ...) is declared in editor_state.h and defined
in editor_mode.cpp, since that's also where onRender() needs it.
*/
void EditorMode::onEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.scancode) {
            /*
            Esc opens the settings screen (see settings_mode.h's class
            comment on setReturnMode); Q is the only thing that quits,
            consistently across every mode.
            */
            case SDL_SCANCODE_ESCAPE:
                SettingsMode::setReturnMode(SettingsMode::ReturnMode::Editor);
                context_.switchMode(std::make_unique<SettingsMode>());
                return;

            case SDL_SCANCODE_Q:
                context_.requestQuit();
                return;

            /*
            Ground/Objects/Entities are not manually selectable —
            activeLayer always follows whichever palette tile is selected
            (see handlePointerAction's click-to-select-layer branch), so
            it's never possible to paint a tile onto the "wrong" layer.
            The four marker tools (Collision, Stairs down/up, Occlusion)
            have no palette tile of their own, so 1-4 are their dedicated
            keys — each also toggles that layer's overlay implicitly,
            since the overlay is only ever drawn for markers that exist
            regardless of activeLayer, and this is the only key that lets
            you ADD to that layer in the first place.
            */
            case SDL_SCANCODE_1:
                activeLayer         = EditLayer::COLLISION;
                activeCollisionTool = CollisionTool::BLOCK;
                editorStatus        = " Placing: Collision marker";
                editorStatusTTL     = 90;
                break;

            // Left-click paints the selected marker, right-click erases it
            // (erase doesn't care which marker was there — see eraseTile).
            case SDL_SCANCODE_2:
                activeLayer         = EditLayer::COLLISION;
                activeCollisionTool = CollisionTool::STAIRS_DOWN;
                editorStatus        = " Placing: Stairs DOWN marker";
                editorStatusTTL     = 90;
                break;
            case SDL_SCANCODE_3:
                activeLayer         = EditLayer::COLLISION;
                activeCollisionTool = CollisionTool::STAIRS_UP;
                editorStatus        = " Placing: Stairs UP marker";
                editorStatusTTL     = 90;
                break;

            /*
            Occlusion layer — same idea as 1-3: pick the tool, then
            left-click paints one OCCLUSION_MARKER cell (right-click
            erases it) — no auto-stamping, same single-cell behavior as
            the collision markers above.
            */
            case SDL_SCANCODE_4:
                activeLayer      = EditLayer::OCCLUSION;
                editorStatus     = " Placing: Occlusion marker";
                editorStatusTTL  = 90;
                break;

            /*
            World location selection
            Steps selectedCoord — the F5/F6 target — one at a time. Works
            whether or not that coordinate has a saved file yet, which is
            how new locations get created: step to an unused spot, draw,
            F5. Clicking a row in the right-panel list jumps here too.
            */
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                --selectedCoord.x;
                break;

            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                ++selectedCoord.x;
                break;

            case SDL_SCANCODE_W:
            case SDL_SCANCODE_UP:
                --selectedCoord.y;
                break;

            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
                ++selectedCoord.y;
                break;

            case SDL_SCANCODE_PAGEUP:
                ++selectedCoord.floor;
                break;

            case SDL_SCANCODE_PAGEDOWN:
                --selectedCoord.floor;
                break;

            case SDL_SCANCODE_F5:
                saveEditorToLocation(selectedCoord);
                break;

            case SDL_SCANCODE_F9:
                loadLocationIntoEditor(selectedCoord);
                break;

            default: break;
        }
        return;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT)  mouseDown_      = true;
        if (e.button.button == SDL_BUTTON_RIGHT) rightMouseDown_ = true;
        handlePointerEvent(e.button.x, e.button.y);
        return;
    }

    if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_LEFT)  mouseDown_      = false;
        if (e.button.button == SDL_BUTTON_RIGHT) rightMouseDown_ = false;
        return;
    }

    if (e.type == SDL_MOUSEMOTION) {
        if (mouseDown_ || rightMouseDown_) handlePointerEvent(e.motion.x, e.motion.y);
        return;
    }

    if (e.type == SDL_MOUSEWHEEL) {
        // SDL2's wheel event carries no pointer position of its own — query
        // it separately to tell which panel is under the cursor right now.
        int winMx = 0, winMy = 0;
        SDL_GetMouseState(&winMx, &winMy);

        int winW = 0, winH = 0;
        SDL_GetWindowSize(getWindow(), &winW, &winH);

        int lx = 0, ly = 0;
        bool onCanvas = displayWindowToLogical(winMx, winMy, winW, winH, lx, ly);

        if (onCanvas && lx >= RIGHT_PANEL_X) {
            worldListScroll -= e.wheel.y * CELL_SIZE * 3;
            if (worldListScroll < 0) worldListScroll = 0;
        } else {
            paletteScroll -= e.wheel.y * PALETTE_CELL * 3;
            if (paletteScroll < 0) paletteScroll = 0;
        }
        return;
    }
}

#include "settings_mode.h"

#include <algorithm>
#include <vector>

#include "../core/display.h"
#include "../core/layout.h"
#include "../core/renderer.h"
#include "../editor/editor_mode.h"
#include "../game/game_mode.h"
#include "settings_panel.h"

namespace {

/*
Categories
Only "Resolution" actually does anything. "Volume" is listed (per the
request to leave room for it) but has no audio system behind it yet —
see onEvent's SPACE handling below. A real category is added by
appending here and giving it its own branch in the two switch-like
blocks in onEvent()/onRender() — index-matched, so nothing else changes.
*/
const std::vector<std::string> kCategories = { "Resolution", "Volume" };
constexpr int kCategoryResolution = 0;
constexpr int kCategoryVolume     = 1;

/*
0 = left column (categories) has keyboard focus, 1 = right column (that
category's options). Whichever column is focused is drawn in yellow — see
layout.h's DIVIDER_W comment style: this is the one piece of state that
isn't per-category, so it lives at file scope same as EditorMode's
activeLayer rather than needing its own struct.
*/
int focusedColumn_ = 1;
int categoryIndex_ = kCategoryResolution;

/*
Remembers which option is highlighted PER category, so switching from
Resolution to Volume and back doesn't lose your place in either list.
Sized/reset to kCategories.size() in onEnter().
*/
std::vector<int> optionIndex_;

// Transient status line, same TTL convention as EditorMode's editorStatus —
// currently only used for Volume's "not implemented yet" note.
std::string statusText_;
int         statusTTL_ = 0;

/*
See settings_mode.h's class comment — which mode ESC returns to. File-
scope rather than a SettingsMode member since a fresh instance is
constructed every time App::switchMode() opens this screen (see
core/app.cpp), which would otherwise reset it right back to the default
before onEvent() ever got a chance to read it.
*/
SettingsMode::ReturnMode returnMode_ = SettingsMode::ReturnMode::Game;

std::vector<std::string> resolutionOptions() {
    return displayResolutionNames();
}

/*
Options list for whichever category is currently selected. Returned by
value (these lists are tiny) so callers don't need to care that Volume's
is a single static placeholder while Resolution's is computed fresh.
*/
std::vector<std::string> optionsForCategory(int category) {
    if (category == kCategoryResolution) return resolutionOptions();
    return { SettingsPanel::VOLUME_PLACEHOLDER };
}

int clampIndex(int index, int count) {
    if (count <= 0) return 0;
    return std::max(0, std::min(index, count - 1));
}

} // namespace

void SettingsMode::setReturnMode(ReturnMode mode) {
    returnMode_ = mode;
}

void SettingsMode::onEnter() {
    optionIndex_.assign(kCategories.size(), 0);

    /*
    Nice-to-have: land on whichever preset matches the window's current
    size, so re-entering settings after already picking a resolution
    shows it highlighted instead of always resetting to the first preset.
    */
    int winW = 0, winH = 0;
    SDL_GetWindowSize(getWindow(), &winW, &winH);
    std::vector<std::string> names = resolutionOptions();
    for (size_t i = 0; i < names.size(); ++i) {
        Resolution r;
        if (displayFindResolution(names[i], r) && r.w == winW && r.h == winH) {
            optionIndex_[kCategoryResolution] = (int)i;
            break;
        }
    }

    focusedColumn_ = 1;
    categoryIndex_ = kCategoryResolution;
    statusText_.clear();
    statusTTL_ = 0;
}

void SettingsMode::onEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    switch (e.key.keysym.scancode) {
        /*
        ESC toggles this screen closed, back to whichever mode opened it
        (see setReturnMode()'s comment) — Q is the only thing that quits,
        consistently across every mode (see game_controls.cpp /
        editor_controls.cpp).
        */
        case SDL_SCANCODE_ESCAPE:
            if (returnMode_ == ReturnMode::Editor)
                context_.switchMode(std::make_unique<EditorMode>());
            else
                context_.switchMode(std::make_unique<GameMode>());
            return;

        case SDL_SCANCODE_Q:
            context_.requestQuit();
            return;

        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            focusedColumn_ = 0;
            return;

        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            focusedColumn_ = 1;
            return;

        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN: {
            int delta =
                (e.key.keysym.scancode == SDL_SCANCODE_W ||
                 e.key.keysym.scancode == SDL_SCANCODE_UP)
                    ? -1
                    : 1;
            if (focusedColumn_ == 0) {
                categoryIndex_ = clampIndex(categoryIndex_ + delta, (int)kCategories.size());
            } else {
                int count = (int)optionsForCategory(categoryIndex_).size();
                optionIndex_[categoryIndex_] = clampIndex(optionIndex_[categoryIndex_] + delta, count);
            }
            return;
        }

        case SDL_SCANCODE_SPACE: {
            if (focusedColumn_ != 1) return; // SPACE only applies an OPTION, not a category

            if (categoryIndex_ == kCategoryResolution) {
                std::vector<std::string> names = resolutionOptions();
                int idx = clampIndex(optionIndex_[categoryIndex_], (int)names.size());
                Resolution res;
                if (!displayFindResolution(names[idx], res)) return;

                /*
                Presets never actually go below the floor (see
                core/display.h's kPresets), so this can't realistically
                trigger from the settings list — kept as a defensive
                check, same floor /resolution enforces conceptually.
                */
                if (!displayFitsMinimum(res.w, res.h)) {
                    statusText_ = " " + names[idx] + " is smaller than the minimum \u2014 unchanged";
                    statusTTL_  = 150;
                    return;
                }

                SDL_SetWindowSize(getWindow(), res.w, res.h);
                SDL_SetWindowPosition(getWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                statusText_ = " Resolution set to " + names[idx];
                statusTTL_  = 150;
            } else {
                statusText_ = SettingsPanel::VOLUME_STATUS;
                statusTTL_  = 150;
            }
            return;
        }

        default:
            return;
    }
}

void SettingsMode::onRender() {
    int w = 0, h = 0;
    getCanvasSize(w, h);

    /*
    Frame
    A single full-window border — no side panels, no map, so no interior
    dividers to get wrong this time (see game_mode.cpp's DIVIDER_W fix for
    what happens when a divider isn't given its own dedicated column).
    */
    {
        FrameBuilder fb;
        fb.markRow(0, 0, w);
        fb.markRow(h - CELL_SIZE, 0, w);
        fb.markCol(0, 0, h);
        fb.markCol(w - CELL_SIZE, 0, h);
        fb.draw();
    }

    drawStringPx(SettingsPanel::TITLE, SettingsPanel::COL_LEFT * CELL_SIZE,
                 SettingsPanel::ROW_TITLE * CELL_SIZE);
    drawStringPx(SettingsPanel::HINT, SettingsPanel::COL_LEFT * CELL_SIZE,
                 SettingsPanel::ROW_HINT * CELL_SIZE);

    drawStringPx(SettingsPanel::HEADER_SETTING, SettingsPanel::COL_LEFT * CELL_SIZE,
                 SettingsPanel::ROW_HEADERS * CELL_SIZE);
    drawStringPx(SettingsPanel::HEADER_OPTION, SettingsPanel::COL_RIGHT * CELL_SIZE,
                 SettingsPanel::ROW_HEADERS * CELL_SIZE);

    const SDL_Color kFocused = SDL_Color{255, 255, 0, 255};
    const SDL_Color kPlain   = SDL_Color{200, 200, 200, 255};

    // Categories (left column)
    for (size_t i = 0; i < kCategories.size(); ++i) {
        bool isSelected = ((int)i == categoryIndex_);
        SDL_Color color = (isSelected && focusedColumn_ == 0) ? kFocused : kPlain;
        std::string prefix = isSelected ? " > " : "   ";
        drawStringPx(prefix + kCategories[i],
                     SettingsPanel::COL_LEFT * CELL_SIZE,
                     (SettingsPanel::ROW_LIST_START + (int)i) * CELL_SIZE, color);
    }

    // Options (right column) — whichever category is currently selected
    std::vector<std::string> options = optionsForCategory(categoryIndex_);
    int selectedOption = clampIndex(optionIndex_[categoryIndex_], (int)options.size());
    for (size_t i = 0; i < options.size(); ++i) {
        bool isSelected = ((int)i == selectedOption);
        SDL_Color color = (isSelected && focusedColumn_ == 1) ? kFocused : kPlain;
        std::string prefix = isSelected ? " > " : "   ";
        drawStringPx(prefix + options[i],
                     SettingsPanel::COL_RIGHT * CELL_SIZE,
                     (SettingsPanel::ROW_LIST_START + (int)i) * CELL_SIZE, color);
    }

    // Status line
    if (statusTTL_ > 0) {
        int statusRow = SettingsPanel::ROW_LIST_START + (int)std::max(kCategories.size(), options.size()) + 2;
        drawStringPx(statusText_, SettingsPanel::COL_LEFT * CELL_SIZE, statusRow * CELL_SIZE,
                     SDL_Color{130, 230, 130, 255});
        --statusTTL_;
    }
}

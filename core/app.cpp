#include "app.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "display.h"
#include "layout.h"
#include "level.h"
#include "renderer.h"

#include "../editor/editor_mode.h"
#include "../game/game_mode.h"
#include "../settings/settings_mode.h"

namespace {

std::vector<std::string> splitCommand(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

} // namespace

App::App() {
    registerConsoleCommands();
}

void App::registerConsoleCommands() {
    console_.root().children = {
        { "mode", {
            { "game",     {}, nullptr },
            { "editor",   {}, nullptr },
            { "settings", {}, nullptr },
          }, nullptr },
        { "zoom", {
            { "1", {}, nullptr }, { "2", {}, nullptr }, { "3", {}, nullptr }, { "4", {}, nullptr },
          }, nullptr },
        /*
        Dynamic children list every currently loaded "floor-x-y" location,
        e.g. "0-0-0" — so TAB-completing /load offers real coordinates
        instead of requiring them to be typed from memory.
        */
        { "load", {}, [] {
            std::vector<std::string> names;
            for (const LevelCoord& c : listWorldLevels())
                names.push_back(std::to_string(c.floor) + "-" + std::to_string(c.x) + "-" + std::to_string(c.y));
            return names;
          } },
        { "exit", {}, nullptr },
    };

    console_.setSubmitHandler([this](const std::string& line) { dispatchCommand(line); });
}

void App::dispatchCommand(const std::string& line) {
    const SDL_Color kErr = {255, 110, 110, 255};
    const SDL_Color kOk  = {130, 230, 130, 255};

    /*
    Command convention
    Commands are matched literally WITH their leading `/` (tokens[0] ==
    "/mode", not "mode") — so a line that doesn't start with `/` just
    fails every comparison below and falls through to "Unknown command"
    like any other typo. No dedicated check, no dedicated warning: the
    slash is simply part of the word.
    */
    std::vector<std::string> tokens = splitCommand(line);
    if (tokens.empty()) return;

    if (tokens[0] == "/exit") {
        requestQuit();
        return;
    }

    if (tokens[0] == "/mode") {
        if (tokens.size() < 2) {
            console_.setStatusLine(" Usage: /mode game|editor|settings", kErr);
            return;
        }
        if (tokens[1] == "game")             switchMode(std::make_unique<GameMode>());
        else if (tokens[1] == "editor")      switchMode(std::make_unique<EditorMode>());
        else if (tokens[1] == "settings")    switchMode(std::make_unique<SettingsMode>());
        else { console_.setStatusLine(" Unknown mode: " + tokens[1], kErr); return; }

        console_.setStatusLine(" Switched to " + tokens[1], kOk);
        return;
    }

    if (tokens[0] == "/zoom") {
        if (tokens.size() < 2) {
            console_.setStatusLine(" Usage: /zoom <1-4>  (current: " + std::to_string(getZoom()) + ")", kErr);
            return;
        }
        char* end = nullptr;
        long level = std::strtol(tokens[1].c_str(), &end, 10);
        if (end == tokens[1].c_str() || *end != '\0') {
            console_.setStatusLine(" Usage: /zoom <1-4>", kErr);
            return;
        }
        setZoom((int)level);
        needsRender_ = true;
        console_.setStatusLine(" Zoom set to " + std::to_string(getZoom()) + "x", kOk);
        return;
    }

    if (tokens[0] == "/load") {
        if (tokens.size() < 2) {
            console_.setStatusLine(" Usage: /load <floor>-<x>-<y>  (TAB to list known locations)", kErr);
            return;
        }
        int floor = 0, x = 0, y = 0;
        if (sscanf(tokens[1].c_str(), "%d-%d-%d", &floor, &x, &y) != 3) {
            console_.setStatusLine(" Usage: /load <floor>-<x>-<y>, e.g. /load 0-0-0", kErr);
            return;
        }

        /*
        Always (re)enter a fresh GameMode first — this re-scans world/ so
        a location just saved by the editor is picked up immediately, and
        matches how /mode game already behaves. travelTo() then overrides
        the spawn point to the requested coordinate.
        */
        switchMode(std::make_unique<GameMode>());
        if (!GameMode::travelTo(floor, x, y)) {
            console_.setStatusLine(" No location loaded at " + tokens[1], kErr);
            return;
        }

        console_.setStatusLine(" Loaded location " + tokens[1], kOk);
        return;
    }

    console_.setStatusLine(" Unknown command: " + line, kErr);
}

void App::switchMode(std::unique_ptr<IMode> next) {
    if (mode_) mode_->onExit();
    mode_ = std::move(next);
    if (mode_) {
        AppContext ctx;
        ctx.requestQuit = [this] { requestQuit(); };
        ctx.switchMode  = [this](std::unique_ptr<IMode> m) { switchMode(std::move(m)); };
        mode_->setContext(std::move(ctx));
        mode_->onEnter();
    }
    needsRender_ = true;
}

void App::requestQuit() { running_ = false; }

void App::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) { running_ = false; return; }

    if (console_.onEvent(e)) { needsRender_ = true; return; }

    if (mode_) mode_->onEvent(e);
    needsRender_ = true;
}

void App::run() {
    initSDL();
    srand(static_cast<unsigned>(time(nullptr)));
    initLevels();

    /*
    A fresh launch starts on the settings screen at the smallest named
    resolution preset, so there's always a real (named, re-selectable)
    resolution active before the player ever sees the map — rather than
    the raw MIN_WINDOW_W x MIN_WINDOW_H floor initSDL() sizes the window
    to, which isn't itself one of the /mode settings options. ESC from
    here (see SettingsMode::onEvent) is what actually gets the player
    into GameMode.
    */
    const std::vector<Resolution>& presets = displayResolutionPresets();
    if (!presets.empty()) {
        SDL_SetWindowSize(getWindow(), presets.front().w, presets.front().h);
        SDL_SetWindowPosition(getWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    switchMode(std::make_unique<SettingsMode>());

    while (running_) {
        if (needsRender_) {
            /*
            Every mode draws at the one fixed CANVAS_W x CANVAS_H virtual
            resolution (core/layout.h) regardless of window size —
            endFrame() scales the finished frame to fit the real window.
            */
            beginFrame();
            if (mode_) mode_->onRender();
            console_.render();
            endFrame();
            needsRender_ = false;
        }

        SDL_Event e;
        if (!SDL_WaitEvent(&e)) continue;
        handleEvent(e);
    }

    if (mode_) mode_->onExit();
    cleanupSDL();
}

#pragma once

#include <memory>

#include <SDL2/SDL.h>

#include "console.h"
#include "mode.h"

/*
Owns the whole SDL lifecycle: window, renderer, main loop, the current
IMode, and the console overlay. This is the one "monolithic window" object —
GameMode and EditorMode never touch SDL_CreateWindow themselves, they only
draw into the frame App has already begun.
*/
class App {
public:
    App();

    void run();

    // Called by the console's /mode handler (see registerConsoleCommands()).
    void switchMode(std::unique_ptr<IMode> next);
    void requestQuit();

private:
    void handleEvent(const SDL_Event& e);
    void registerConsoleCommands();
    void dispatchCommand(const std::string& line);

    bool                   running_     = true;
    bool                   needsRender_ = true;
    std::unique_ptr<IMode> mode_;
    Console                console_;
};

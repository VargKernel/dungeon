#pragma once

#include <vector>

#include "../core/mode.h"
#include "../core/tiles.h"

/*
GameMode keeps its player position, map layers, and other per-session
state as extern globals (game_state.h) rather than instance members, so
that state has whole-program storage duration and survives a round trip
through EditorMode intact — switching modes never resets it.

onEnter()/onRender() (state init + drawing) are defined in game_mode.cpp;
onEvent() (all keyboard input handling) is defined in game_controls.cpp —
both are member functions of this one class, split across two .cpp files
that share state through game_state.h.
*/
class GameMode : public IMode {
public:
    void onEnter() override;
    void onEvent(const SDL_Event& e) override;
    void onRender() override;

    /*
    Teleports to the world location at (floor, x, y), entering at its
    authored default spawn point. Used by the console's /load command.
    Returns false, leaving the current location untouched, if nothing is
    loaded at that coordinate. Safe to call whether or not GameMode is
    the currently active IMode (state lives in file-scope statics — see
    the comment below).
    */
    static bool travelTo(int floor, int x, int y);

private:
    void render();

    /*
    Steps an Objects-layer anchor through frames[1..], forcing a real
    frame + short delay between each step (see interactions.h) so multi-
    frame object animations (chests opening, etc.) are actually visible
    instead of snapping straight to the end state. frames[0] must equal
    whatever's currently at (ax, ay).
    */
    void playAnimation(int ax, int ay, const std::vector<TileID>& frames);
};

#pragma once

#include "../core/mode.h"

/*
EditorMode runs inside App's shared SDL_WaitEvent loop rather than owning
its own window or poll loop, so it reacts to real
SDL_MOUSEBUTTONDOWN/SDL_MOUSEMOTION events instead of re-evaluating "is
the mouse button down, where is it now" every frame — click-and-drag
painting works by re-painting on every motion event while a button is
held, driven entirely by events rather than a per-frame poll. See
game_mode.h's comment for why editor state lives in extern globals
(editor_state.h) rather than as members here — same reasoning applies.
onEnter()/onRender() are defined in editor_mode.cpp; onEvent() (all
keyboard/mouse input handling) is defined in editor_controls.cpp — both
are member functions of this one class, split across two .cpp files that
share state through editor_state.h.
*/
class EditorMode : public IMode {
public:
    void onEnter() override;
    void onEvent(const SDL_Event& e) override;
    void onRender() override;
};

#pragma once

#include "../core/mode.h"

/*
A third IMode alongside GameMode and EditorMode — see core/mode.h. A real
settings screen: one full-window panel, framed on all four sides, with no
map and no left/right split. Two columns — SETTING (category) and OPTION
(that category's values) — navigated with AWSD and confirmed with SPACE;
the currently focused entry (whichever column has keyboard focus) is
drawn in yellow, same convention EditorMode uses for its selected palette
tile.

App::run() enters this mode FIRST, before GameMode, so a fresh launch
always starts by letting the player confirm/choose a resolution — see
that comment for why the window itself starts pinned to the smallest
preset rather than whatever the OS handed back.

Currently only the Resolution category is actually interactive; Volume is
listed (per the request to leave room for it) but has no real audio
system behind it yet — see settings_mode.cpp's onEvent for both.

ESC toggles this screen open/closed from ANY mode (Game/EditorMode's own
ESC handlers just call context_.switchMode(make_unique<SettingsMode>())
— see game_controls.cpp / editor_controls.cpp) rather than quitting; Q is
the only thing that quits, in every mode. Settings itself doesn't know
which mode it was opened FROM on its own, so the opening mode records
that here via setReturnMode() right before switching — ESC pressed while
already in Settings reads it back to know whether to return to Game or
Editor. Defaults to Game so the very first launch (App::run() switches
straight into Settings with nothing set) still has a sane ESC target.
*/
class SettingsMode : public IMode {
public:
    enum class ReturnMode { Game, Editor };

    /*
    Called by GameMode/EditorMode's ESC handler right before switching
    into this mode, so this mode's own ESC handler knows where "back"
    goes. Not a constructor parameter because App::switchMode() only
    calls the default no-argument onEnter() — this is simpler than
    threading a parameter through AppContext for a single enum value.
    */
    static void setReturnMode(ReturnMode mode);

    void onEnter() override;
    void onEvent(const SDL_Event& e) override;
    void onRender() override;

    /*
    Single full-window panel — no fixed map content to flank, unlike
    Game/EditorMode's shared three-column layout. Draws at the same fixed
    CANVAS_W x CANVAS_H virtual resolution as every other mode (see
    core/layout.h) — see settings_mode.cpp's onRender().
    */
};

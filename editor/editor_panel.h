#pragma once

#include <string>

/*
EditorMode panel layout
Row indices for the text drawn into the info box below the map (see
EditorMode::onRender() in editor_mode.cpp). Row 0 is the panel's own top
row; UI_BOX_ROWS (core/layout.h) is 13, so row indices here must stay
within [0, 11] to leave the box's last row as a clean bottom margin — 12
rows is the hard ceiling.

Purely the STATIC layout/text — anything that depends on live state
(which layer is active, the transient save/load status message, the
current world coordinate) still gets assembled in editor_mode.cpp, since
only that file has the data; this file just says which row it goes on
and supplies the wording that never changes at runtime.
*/
namespace EditorPanel {

    // Info box rows
    constexpr int ROW_STATUS          = 2; // transient F5/F6 save-load status
    constexpr int ROW_HELP_PLACE      = 4;
    constexpr int ROW_HELP_COLLISION  = 5;
    constexpr int ROW_HELP_LOCATION   = 6;
    constexpr int ROW_HELP_SAVE       = 7;
    constexpr int ROW_HELP_QUIT       = 8;

    // Right panel (world location list) rows
    constexpr int ROW_WORLD_HEADER = 1; // " Floor N (x, y) [new]"
    constexpr int ROW_WORLD_LEGEND = 2; // arrow-key / PageUp/PageDown legend
    /*
    The list itself starts two rows down (header + legend), same offset
    used by both the click hit-test (editor_controls.cpp) and the render
    (editor_mode.cpp) — kept as a named constant so the two can't drift
    apart from each other.
    */
    constexpr int WORLD_LIST_START_ROW = 2;

    // Static label text
    extern const std::string WORLD_LEGEND;
    extern const std::string HELP_PLACE;
    extern const std::string HELP_COLLISION;
    extern const std::string HELP_LOCATION;
    extern const std::string HELP_SAVE;
    extern const std::string HELP_QUIT;

} // namespace EditorPanel

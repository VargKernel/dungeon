#pragma once

#include <string>

/*
SettingsMode panel layout
Row/column indices (grid cells, not pixels) for the single full-window
panel SettingsMode draws — see settings_mode.cpp's onRender(). Unlike
GamePanel/EditorPanel there's no fixed-size map to work around, so these
are just "where things go" rather than a hard row budget.
*/
namespace SettingsPanel {

    constexpr int ROW_TITLE       = 1;
    constexpr int ROW_HINT        = 2;
    constexpr int ROW_HEADERS     = 4;
    constexpr int ROW_LIST_START  = 5;

    constexpr int COL_LEFT  = 2;  // "SETTING" column (categories)
    constexpr int COL_RIGHT = 20; // "OPTION" column (that category's values)

    extern const std::string TITLE;
    extern const std::string HINT;
    extern const std::string HEADER_SETTING;
    extern const std::string HEADER_OPTION;
    extern const std::string VOLUME_PLACEHOLDER;
    extern const std::string VOLUME_STATUS;

} // namespace SettingsPanel

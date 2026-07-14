#pragma once

#include <string>
#include <vector>

/*
GameMode info panel layout
Text drawn into the info box below the map (see GameMode::render() in
game_mode.cpp) and into the two side panels. All three follow the same
shape: a std::vector<std::string>, one row per entry, top to bottom,
drawn by game_mode.cpp's drawPanelLines(). The info box is capped at 12
rows (row 11 as bottom margin — 12 is the hard ceiling, see
core/layout.h's UI_BOX_ROWS); the side panels run the full canvas height
and can hold as many rows as needed — the left panel's stat block in
particular is open-ended (see buildLeftPanelLines below).
*/
namespace GamePanel {

    /*
    Builds the info box's rows fresh, every call — message goes straight
    into its own row instead of a placeholder that gets overwritten
    afterward; the rest are still plain placeholder text. See
    game_mode.cpp's render() for the call site.
    */
    std::vector<std::string> buildInfoBoxLines(const std::string& message);

    /*
    Left panel: location + stat block
    location gets LOCATION_LABEL_PREFIX prepended automatically. statLines
    is however many pre-built "icon ABBR - value" lines the caller wants —
    see buildStatLine() and the ICON_* constants below — placed directly
    after location, one per row; there's no fixed count, the panel is
    just as tall as it needs to be. zoomLabel is the last of the "live
    data" rows; everything after it is plain placeholder text.
    */
    extern const std::string LOCATION_LABEL_PREFIX; // " Location: "
    std::vector<std::string> buildLeftPanelLines(const std::string& location,
                                                  const std::vector<std::string>& statLines,
                                                  const std::string& zoomLabel);

    /*
    One "icon ABBR - value" stat row, e.g. buildStatLine(ICON_HP, "HP",
    "100/100") -> " \uf004 HP - 100/100". Icons are plain Unicode
    characters, chosen so the row renders correctly with any font.
    */
    std::string buildStatLine(const std::string& icon, const std::string& abbr, const std::string& value);

    extern const std::string ICON_HP;  // Health
    extern const std::string ICON_STA; // Stamina — rolls, sprinting, heavy attacks
    extern const std::string ICON_MP;  // Mana — spells
    extern const std::string ICON_ATK; // Attack — physical damage
    extern const std::string ICON_DEF; // Defense — damage reduction
    extern const std::string ICON_DEX; // Dexterity — attack speed, bows, dodge chance
    extern const std::string ICON_INT; // Intelligence — magic damage
    extern const std::string ICON_CHA; // Charisma
    extern const std::string ICON_LCK; // Luck — loot, crits, rare events

    // Right panel
    extern const std::vector<std::string> RIGHT_PANEL_LINES;

    // Static label text
    extern const std::string CONTROLS_MOVE;
    extern const std::string CONTROLS_MISC;
    extern const std::string ZOOM_LABEL_PREFIX; // " Zoom: " — sandwiches getZoom()
    extern const std::string ZOOM_LABEL_SUFFIX; // "x"

} // namespace GamePanel

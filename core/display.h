#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

#include "layout.h"

/*
Window resolution & scale
The game always renders a fixed CANVAS_W x CANVAS_H logical frame (see
layout.h) — this module's only job is figuring out how to blit that one
finished frame into whatever the real SDL_Window size is, without ever
stretching it or changing its aspect ratio:

  scale = min(windowW / CANVAS_W, windowH / CANVAS_H)

scale is a plain float, not an integer, so the canvas is scaled by that
single factor and centered; any leftover space on the other axis is
letterboxed (top/bottom or left/right bars) in the same background color
the canvas itself clears to, so it reads as part of the interface, not a
bug.

This module does no SDL_Window calls itself — it only computes numbers.
core/app.cpp calls SDL_SetWindowSize when the resolution changes, and
core/renderer.cpp's endFrame() calls displayComputeDestRect() every frame
to know where to blit the logical canvas.
*/

/*
The smallest window resolution the app supports — see
instructions/minimum resolution.txt. Below this floor the layout is no
longer guaranteed to be comfortably usable; presets never go smaller than
this, and /resolution refuses anything smaller.
*/
constexpr int MIN_WINDOW_W = 1264;
constexpr int MIN_WINDOW_H = 800;

struct Resolution {
    int         w;
    int         h;
    const char* label; // e.g. "1920x1080" — also the name used in /set resolution
};

// Hardcoded presets for `/set resolution <name>`, ascending by size. The
// first entry is always exactly MIN_WINDOW_W x MIN_WINDOW_H.
const std::vector<Resolution>& displayResolutionPresets();
bool                            displayFindResolution(const std::string& name, Resolution& out);

// Names for console/settings autocomplete, in preset order.
std::vector<std::string> displayResolutionNames();

/*
True if windowW x windowH is at least MIN_WINDOW_W x MIN_WINDOW_H — the
one floor SettingsMode's /resolution picker enforces before calling
SDL_SetWindowSize. Unlike the old version, this is unrelated to the
canvas size — the canvas is fixed and simply scales down (never below a
sane minimum in practice, since presets never go below the floor) rather
than ever being "too small to fit".
*/
bool displayFitsMinimum(int windowW, int windowH);

/*
The (possibly fractional) scale factor that fits the fixed CANVAS_W x
CANVAS_H canvas inside windowW x windowH while preserving aspect ratio.
Purely a function of the current window size — always "auto".
*/
float displayComputeScale(int windowW, int windowH);

/*
Where to blit the CANVAS_W x CANVAS_H logical canvas inside the window:
scaled by displayComputeScale and centered, with letterbox/pillarbox bars
filling whatever's left over on the other axis. Rounded down so the
destination rect never exceeds the window on either axis.
*/
SDL_Rect displayComputeDestRect(int windowW, int windowH);

/*
Transforms real-window pixel coordinates (e.g. from an SDL mouse event)
into logical-canvas pixel coordinates, accounting for the current
scale/letterbox. Returns false — and leaves outX/outY untouched — if the
point falls in the letterbox border rather than on the canvas itself.
*/
bool displayWindowToLogical(int windowX, int windowY, int windowW, int windowH,
                             int& outX, int& outY);

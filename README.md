<div align="center">

# Dungeon (work in progress...)

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey?logo=linux&logoColor=white)](#building)
[![Windows (MinGW)](https://img.shields.io/badge/platform-Windows-1A1A2E?logo=mingww64&logoColor=white)](#building)
[![CMake](https://img.shields.io/badge/CMake-build-064F8C?logo=cmake)](https://cmake.org/)
[![SDL 2](https://img.shields.io/badge/SDL-2-1A1A2E?logo=sdl)](https://www.libsdl.org/)

[![Debian 12 (Bookworm)](https://github.com/VargKernel/dungeon/actions/workflows/build-debian-12.yml/badge.svg)](https://github.com/VargKernel/dungeon/actions/workflows/build-debian-12.yml)
[![Debian 13 (Trixie)](https://github.com/VargKernel/dungeon/actions/workflows/build-debian-13.yml/badge.svg)](https://github.com/VargKernel/dungeon/actions/workflows/build-debian-13.yml)
[![Ubuntu 22.04 (Jammy)](https://github.com/VargKernel/dungeon/actions/workflows/build-ubuntu-22.04.yml/badge.svg)](https://github.com/VargKernel/dungeon/actions/workflows/build-ubuntu-22.04.yml)
[![Ubuntu 24.04 (Noble)](https://github.com/VargKernel/dungeon/actions/workflows/build-ubuntu-24.04.yml/badge.svg)](https://github.com/VargKernel/dungeon/actions/workflows/build-ubuntu-24.04.yml)
[![Windows (MinGW cross-compile)](https://github.com/VargKernel/dungeon/actions/workflows/build-windows-mingw.yml/badge.svg)](https://github.com/VargKernel/dungeon/actions/workflows/build-windows-mingw.yml)

**A tile-based dungeon crawler with a built-in level editor**

</div>

Written in C++17 and SDL2, `dungeon` runs the game and the level editor inside the same window and shares a single renderer, switching between modes (`IMode`) without restarting the application.

## Features

- 🗺️ Gameplay mode with a five-layer map (Ground / Object / Entity / Collision / Occlusion), interactive objects (doors, chests, cabinets, barrels, vases), and seamless transitions between locations through map edges and staircases.
- 🛠️ Built-in level editor with a tile palette, collision/stair/occlusion tools, world location browser, and JSON-based map saving/loading.
- ⚙️ Settings screen with selectable window resolutions.
- 💻 Integrated developer console (`` ` ``) featuring `/mode`, `/zoom`, `/load`, `/exit`, and Tab auto-completion.
- 🖥️ Fixed virtual canvas (1280×800) with automatic scaling and letterboxing for any window resolution.

## Building

### Scripts Overview

| Script | Purpose | What it does |
| --- | --- | --- |
| `setup.sh` | One-time environment setup | Installs Linux build dependencies (CMake, SDL2, SDL2_image, SDL2_ttf, MinGW) via `apt`, and downloads the MinGW builds of SDL2, SDL2_image, and SDL2_ttf into `win_libs/` for Windows cross-compilation. Run once before the first build, or after switching machines. |
| `build.sh` | Linux build | Configures the project with CMake and compiles it into `./build`. Creates symbolic links to `assets/` and `world/` inside the build directory so the editor reads and writes the original project data directly, without copying. |
| `build_win.sh` | Windows cross-build | Cross-compiles the project with MinGW into `./build_win`, producing `dungeon.exe`, and copies the required SDL DLLs next to the executable so it runs standalone on Windows. |
| `release.sh` | Linux release packaging | Builds a standalone distribution in `release/`: replaces the `assets/` and `world/` symlinks with real copies, strips temporary CMake artifacts, and leaves a directory ready to ship. |
| `release_win.sh` | Windows release packaging | Same as `release.sh`, but produces a standalone Windows distribution in `release_win/`, including `dungeon.exe`, DLLs, and real copies of `assets/` and `world/`. |

### Linux

Requirements:

- CMake
- C++17-compatible compiler
- SDL2
- SDL2_image
- SDL2_ttf

```bash
./setup.sh   # Installs dependencies via apt (including MinGW for Windows cross-compilation)
./build.sh   # Configures and builds the project in ./build
```

The resulting executable is:

```text
./build/dungeon
```

`build.sh` creates symbolic links to the project's `assets/` and `world/` directories, allowing the editor to work directly with the original project data.

### Windows (Cross-compiling from Linux using MinGW)

```bash
./setup.sh       # Downloads SDL2, SDL2_image, and SDL2_ttf for MinGW into win_libs/
./build_win.sh
```

The resulting files are placed in:

```text
./build_win/dungeon.exe
```

All required SDL DLLs are copied next to the executable.

### Release Builds

`release.sh` and `release_win.sh` create standalone distributions inside `release/` and `release_win/`.

During the release process they:

- replace symbolic links with real copies of `assets/` and `world/`,
- remove temporary CMake files,
- produce a directory ready for distribution.

## Controls

### Game

| Key | Action |
| --- | --- |
| W / A / S / D or Arrow Keys | Move and face the character |
| E | Interact with the object in front of the player |
| Space | Attack or destroy the object in front of the player |
| `+` / `-` | Zoom camera |
| Esc | Open Settings |
| Q | Quit the game |

### Editor

| Key | Action |
| --- | --- |
| 1 / 2 / 3 / 4 | Select tools: Collision / Down Stairs / Up Stairs / Occlusion |
| Left / Right Mouse Button | Paint / Erase tiles or markers |
| A / D, W / S (or Arrow Keys) | Change selected world location (X / Y) |
| Page Up / Page Down | Change selected floor |
| F5 | Save the current map to the selected world location |
| F9 | Load the selected world location |
| Mouse Wheel | Scroll the tile palette or world location list (depending on the hovered panel) |
| Esc | Open Settings |
| Q | Quit the game |

### Console

Open the console at any time by pressing the `` ` `` (grave) key.

| Command | Action |
| --- | --- |
| `/mode game\|editor\|settings` | Switch application mode |
| `/zoom <1-4>` | Set camera zoom level |
| `/load <floor>-<x>-<y>` | Load and switch to the specified location (for example `/load 0-0-0`) |
| `/exit` | Quit the game |

Tab provides auto-completion for commands and their arguments, including existing world locations for `/load`.

## Project Structure

```text
core/       Shared infrastructure: virtual canvas, layout, renderer, window scaling,
            console, tile metadata, level format, and world (world/*.json)

game/       Gameplay mode: game state, input handling, interactions, HUD

editor/     Level editor: editor state, input handling, editor UI, tile palette

settings/   Settings screen

main.cpp    Application entry point

assets/     Sprites and fonts

world/      Saved world locations (JSON, one file per location)

docs/       ASCII interface mockups and design notes
```

Each application mode (`GameMode`, `EditorMode`, `SettingsMode`) implements the common `IMode` interface (`core/mode.h`) and renders within the fixed 1280×800 virtual canvas (`core/layout.h`), ensuring a consistent interface layout across all modes.

## Assets

Information about third-party assets and their licenses can be found in [CREDITS.md](CREDITS.md).

## License

This project is licensed under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

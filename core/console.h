#pragma once

#include <SDL2/SDL.h>

#include <functional>
#include <string>
#include <vector>

/*
Console
Quake-style overlay console, toggled with ` (backtick / SDL_SCANCODE_GRAVE).
While open it owns all keyboard input — App must not forward events to the
active IMode in that case (see core/app.cpp).

Console itself knows nothing about /mode or /set — it's a generic line
editor + autocomplete tree + history. The owner (App) registers the actual
command tree via root() and receives submitted lines via setSubmitHandler().
This keeps Console reusable and free of dependencies on GameMode/EditorMode/
display.h.
*/

// One node of the autocomplete tree, e.g. for "/set resolution 1920x1080":
//   root -> "set" -> "resolution" -> (dynamicChildren = displayResolutionNames)
struct CompletionNode {
    std::string                             name;
    std::vector<CompletionNode>              children;
    std::function<std::vector<std::string>()> dynamicChildren; // optional, evaluated at TAB time
};

class Console {
public:
    Console();

    // App builds the command tree here at startup, e.g.:
    //   root().children.push_back({"mode", {{"game", {}, nullptr}, {"editor", {}, nullptr}}, nullptr});
    CompletionNode& root();

    // Called with the full raw line (as typed, leading '/' included if the
    // user typed one) when the user presses Enter on a non-empty line.
    void setSubmitHandler(std::function<void(const std::string& line)> handler);

    /*
    App calls this after handling a submitted line, to show a one-line
    result (confirmation or rejection reason) under the prompt — e.g.
    "/set resolution 1280x720" gets rejected because it's smaller than the
    logical canvas, and the status line is how the user finds out why
    nothing happened instead of the command silently doing nothing.

    Each call appends a line to a short scrollback log (Minecraft-chat
    style) rather than replacing a single line — so a run of commands
    stays readable instead of only ever showing the very last result.
    `color` lets the caller distinguish success (default, white) from
    e.g. an error (pass a red-tinted color).
    */
    void setStatusLine(const std::string& text, SDL_Color color = SDL_Color{255, 255, 255, 255});

    bool isOpen() const;
    void toggle();

    // Returns true if the event was consumed by the console (App should not
    // forward it to the active IMode when this returns true).
    bool onEvent(const SDL_Event& e);

    // Draws the overlay via core/renderer.h; no-op while closed.
    void render() const;

private:
    void submit();
    void completeTab();
    void navigateHistory(int direction);
    static std::vector<std::string> tokenize(const std::string& line);

    /*
    Walks the tree along `path` (first token's leading '/' is stripped
    automatically) and returns the candidate names at that node — static
    children plus dynamicChildren(), if any. Empty if `path` doesn't match
    a known command. Shared by completeTab() and render()'s live hint line.
    */
    std::vector<std::string> candidatesForPath(const std::vector<std::string>& path) const;

    bool                     open_        = false;
    bool                     suppressNextTextInput_ = false; // swallows the '`' TEXTINPUT that follows the GRAVE keydown which opened the console
    std::string              buffer_;

    struct LogLine { std::string text; SDL_Color color; };
    std::vector<LogLine>     log_; // scrollback: echoed commands + their results, oldest first
    static constexpr size_t  kMaxLogLines = 6;
    void pushLog(const std::string& text, SDL_Color color);

    std::vector<std::string> history_;
    int                      historyPos_  = -1; // -1 = not browsing history
    CompletionNode           root_;
    std::function<void(const std::string&)> onSubmit_;
};

#include "console.h"

#include "layout.h"
#include "renderer.h"

Console::Console() {
    root_.name = ""; // root node itself is unnamed
}

CompletionNode& Console::root() { return root_; }

void Console::setSubmitHandler(std::function<void(const std::string&)> handler) {
    onSubmit_ = std::move(handler);
}

void Console::pushLog(const std::string& text, SDL_Color color) {
    log_.push_back({text, color});
    if (log_.size() > kMaxLogLines) log_.erase(log_.begin());
}

void Console::setStatusLine(const std::string& text, SDL_Color color) {
    pushLog(text, color);
}

bool Console::isOpen() const { return open_; }

void Console::toggle() {
    bool wasOpen = open_;
    open_ = !open_;
    if (open_) {
        SDL_StartTextInput();
        if (!wasOpen) {
            // Fresh open — start clean rather than showing whatever was left
            // over from a session the user closed with Escape mid-typing.
            buffer_.clear();
            log_.clear();
        }
    } else {
        SDL_StopTextInput();
    }
}

bool Console::onEvent(const SDL_Event& e) {
    // ` always toggles, whether the console is open or closed.
    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
        bool wasOpen = open_;
        toggle();
        /*
        SDL_StartTextInput() was just enabled by toggle(), but the '`'
        keypress that triggered it still produces its own SDL_TEXTINPUT
        event right behind this one — swallow exactly that one so it
        doesn't land in the buffer as a leading '`'.
        */
        if (!wasOpen && open_) suppressNextTextInput_ = true;
        return true;
    }

    if (!open_) return false;

    if (e.type == SDL_TEXTINPUT) {
        if (suppressNextTextInput_) { suppressNextTextInput_ = false; return true; }
        buffer_ += e.text.text;
        return true;
    }

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_BACKSPACE:
                // ASCII-only assumption — fine for command input, would need
                // UTF-8-aware trimming if the console ever accepted free text.
                if (!buffer_.empty()) buffer_.pop_back();
                break;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                submit();
                break;
            case SDL_SCANCODE_ESCAPE:
                open_ = false;
                SDL_StopTextInput();
                break;
            case SDL_SCANCODE_TAB:
                completeTab();
                break;
            case SDL_SCANCODE_UP:
                navigateHistory(-1);
                break;
            case SDL_SCANCODE_DOWN:
                navigateHistory(1);
                break;
            default:
                break;
        }
        return true;
    }

    // Swallow everything else (mouse, etc.) while open so it never leaks
    // through to the active IMode.
    return true;
}

void Console::submit() {
    if (!buffer_.empty()) {
        history_.push_back(buffer_);
        pushLog("> " + buffer_, SDL_Color{170, 170, 170, 255}); // dim echo, Minecraft-chat style
        if (onSubmit_) onSubmit_(buffer_);
    }
    buffer_.clear();
    historyPos_ = -1;
}

void Console::navigateHistory(int direction) {
    if (history_.empty()) return;

    if (direction < 0) { // Up — older
        if (historyPos_ == -1) historyPos_ = static_cast<int>(history_.size()) - 1;
        else if (historyPos_ > 0) --historyPos_;
    } else {             // Down — newer
        if (historyPos_ == -1) return;
        ++historyPos_;
        if (historyPos_ >= static_cast<int>(history_.size())) {
            historyPos_ = -1;
            buffer_.clear();
            return;
        }
    }
    buffer_ = history_[historyPos_];
}

std::vector<std::string> Console::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : line) {
        if (c == ' ') { tokens.push_back(current); current.clear(); }
        else          { current += c; }
    }
    tokens.push_back(current); // always at least one (possibly empty) token
    return tokens;
}

std::vector<std::string> Console::candidatesForPath(const std::vector<std::string>& path) const {
    const CompletionNode* node = &root_;
    for (size_t i = 0; i < path.size(); ++i) {
        std::string key = path[i];
        if (i == 0 && !key.empty() && key[0] == '/') key = key.substr(1);

        const CompletionNode* next = nullptr;
        for (const CompletionNode& child : node->children) {
            if (child.name == key) { next = &child; break; }
        }
        if (!next) return {}; // path doesn't match a known command
        node = next;
    }

    std::vector<std::string> candidates;
    for (const CompletionNode& child : node->children) candidates.push_back(child.name);
    if (node->dynamicChildren) {
        std::vector<std::string> dyn = node->dynamicChildren();
        candidates.insert(candidates.end(), dyn.begin(), dyn.end());
    }
    return candidates;
}

void Console::completeTab() {
    std::vector<std::string> tokens = tokenize(buffer_);
    if (tokens.empty()) return;

    std::vector<std::string> path(tokens.begin(), tokens.end() - 1);
    std::string partial = tokens.back();

    /*
    Top-level commands only autocomplete once the user has actually typed
    the leading '/' — without this check, an empty buffer or arbitrary
    text would match every top-level command name (they're stored bare in
    the tree) and complete to something the user never asked for, and
    worse, complete to it WITHOUT the slash (see firstTokenHasSlash below),
    producing a line dispatchCommand() would never recognize.
    */
    if (path.empty() && (partial.empty() || partial[0] != '/')) return;

    std::vector<std::string> candidates = candidatesForPath(path);
    if (candidates.empty()) return;

    /*
    The first token may carry a literal leading '/' typed by the user
    (e.g. "/mo"); match against it with the slash stripped, then restore
    the slash when writing the completion back into the buffer.
    */
    bool firstTokenHasSlash = path.empty() && !partial.empty() && partial[0] == '/';
    std::string partialKey  = firstTokenHasSlash ? partial.substr(1) : partial;

    std::vector<std::string> matches;
    for (const std::string& c : candidates) {
        if (c.rfind(partialKey, 0) == 0) matches.push_back(c);
    }
    if (matches.empty()) return;

    std::string completed;
    if (matches.size() == 1) {
        completed = matches[0];
    } else {
        // Complete to the longest common prefix shared by all matches.
        completed = matches[0];
        for (const std::string& m : matches) {
            size_t i = 0;
            while (i < completed.size() && i < m.size() && completed[i] == m[i]) ++i;
            completed.resize(i);
        }
    }

    std::string newLastToken = firstTokenHasSlash ? ("/" + completed) : completed;

    std::string rebuilt;
    for (const std::string& p : path) { rebuilt += p; rebuilt += ' '; }
    rebuilt += newLastToken;
    if (matches.size() == 1) rebuilt += ' '; // chain straight into the next argument

    buffer_ = rebuilt;
}

void Console::render() const {
    if (!open_) return;

    int canvasW = 0, canvasH = 0;
    getCanvasSize(canvasW, canvasH);
    (void)canvasH; // only width is needed for the overlay bar

    /*
    prompt line + live suggestion hint, then the scrollback log below,
    newest entry closest to the prompt (mirrors Minecraft's chat, just
    anchored to the top of the screen instead of the bottom).
    */
    const int overlayRows = 2 + static_cast<int>(kMaxLogLines);
    fillRect(0, 0, canvasW, overlayRows * CELL_SIZE, SDL_Color{0, 0, 0, 180});
    drawString("> " + buffer_, 0, 0);

    std::vector<std::string> tokens = tokenize(buffer_);
    std::vector<std::string> path   = tokens.empty() ? std::vector<std::string>{}
                                                      : std::vector<std::string>(tokens.begin(), tokens.end() - 1);
    std::string partial = tokens.empty() ? "" : tokens.back();
    bool firstTokenHasSlash = path.empty() && !partial.empty() && partial[0] == '/';
    std::string partialKey  = firstTokenHasSlash ? partial.substr(1) : partial;

    /*
    Mirrors the same rule as completeTab(): at the top level, candidates
    only exist once the user has typed '/'. Without this, the live hint
    line would pop up top-level command suggestions for any text typed
    into the console, which is confusing when the line isn't meant to be
    a command at all.
    */
    std::vector<std::string> candidates;
    if (!tokens.empty() && (!path.empty() || firstTokenHasSlash))
        candidates = candidatesForPath(path);

    /*
    Filter live, as-you-type, by whatever's been typed of the current
    token — showing every sibling regardless of partial match (the old
    behaviour) gets noisy once a command has more than a couple of
    children, e.g. `/set res` should narrow to "resolution", not also
    list unrelated siblings under `/set`.
    */
    std::vector<std::string> matches;
    for (const std::string& c : candidates)
        if (c.rfind(partialKey, 0) == 0) matches.push_back(c);

    if (!matches.empty()) {
        /*
        Top-level commands are shown with their leading `/` in the hint —
        same as you'd actually type them — even though the tree itself
        stores bare names (see candidatesForPath's slash-stripping); this
        is purely cosmetic, deeper levels (e.g. "game"/"editor" under
        /mode) aren't commands of their own so they stay bare.
        */
        std::string prefix = path.empty() ? "/" : "";
        std::string hint;
        for (const std::string& m : matches) { hint += prefix; hint += m; hint += "  "; }
        drawString(hint, 0, 1, SDL_Color{230, 220, 120, 255});
    }

    int row = 2;
    for (auto it = log_.rbegin(); it != log_.rend() && row < overlayRows; ++it, ++row)
        drawString(it->text, 0, row, it->color);
}

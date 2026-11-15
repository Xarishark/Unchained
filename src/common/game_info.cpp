#include "app/app.h"

#include <cstdlib>
#include <windows.h>

namespace {

// UNCHAINED_OFFSCREEN set: the game window lives far off-screen, for automated boot tests (screenshots still work
// through PrintWindow). The bring-up builds can strobe between clear colours, so test runs never show on a monitor.
bool Offscreen() {
    static const bool on = std::getenv("UNCHAINED_OFFSCREEN") != nullptr;
    return on;
}

constexpr int kAway = -32000;

void OffscreenWindow(void* hwnd) {
    if (Offscreen())
        SetWindowPos(static_cast<HWND>(hwnd), nullptr, kAway, kAway, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool OffscreenMessage(void*, uint32_t msg, uintptr_t, intptr_t lp, intptr_t*) {
    if (Offscreen() && msg == WM_WINDOWPOSCHANGING) {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lp);
        pos->x = kAway;
        pos->y = kAway;
        pos->flags &= ~UINT(SWP_NOMOVE);
        pos->flags |= SWP_NOACTIVATE;
    }
    return false;  // the host handles it as usual
}

}  // namespace

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Unchained",                // window and splash
    .short_title = L"Unchained",          // message boxes
    .name = L"Unchained",                 // window classes, crash dumps, handover event
    .log = "unchained",                   // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "Unchained"}},  // each recompiled xbe and its exe, for title launches
    .options = {},                        // game settings kept in the settings file
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = nullptr,                      // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = OffscreenWindow,                    // the game window is up
    .message = OffscreenMessage,                  // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = nullptr,                       // changes a polled pad
    .rumble = nullptr,                    // changes a rumble before it is set
};

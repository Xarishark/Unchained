// Keyboard and mouse bindings for port 0, rebindable in the settings file's [keys] table:
//
//   [keys]
//   jump = ["Space"]            # each action takes one or more keys or mouse buttons
//   attack = ["F", "Mouse1"]
//
// Key names: A-Z, 0-9, F1-F12, Up Down Left Right, Space Enter Escape Tab Backspace, LShift RShift LCtrl RCtrl LAlt
// RAlt, Mouse1 (left) Mouse2 (right) Mouse3 (middle) Mouse4 Mouse5. An action with no keys is unbound.
//
// mouse_camera (on by default): while the game window has focus the mouse turns the camera (the right stick), the
// cursor hidden and kept in the window; mouse_sensitivity in percent.
#include "app/app.h"
#include "core/input.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

#include <toml++/toml.hpp>

bool g_mouse_camera = true;
uint32_t g_mouse_sensitivity = 100;

namespace {

enum Bit : uint16_t { kUp = 1, kDown = 2, kLeft = 4, kRight = 8, kStart = 16, kBack = 32, kLs = 64, kRs = 128 };
enum Analog { kA, kB, kX, kY, kBlack, kWhite, kLt, kRt };

struct Action {
    const char* name;
    uint16_t bit;      // a digital button, or 0
    int analog;        // an analog button, or -1
    int stick;         // 0 left stick, 1 right stick, -1 none
    int dx, dy;        // the stick direction (y up)
    std::vector<int> keys;
};

// the defaults, as the README's controls table has them
std::vector<Action> g_actions = {
    {"menu_up", kUp, -1, -1, 0, 0, {VK_UP}},
    {"menu_down", kDown, -1, -1, 0, 0, {VK_DOWN}},
    {"menu_left", kLeft, -1, -1, 0, 0, {VK_LEFT}},
    {"menu_right", kRight, -1, -1, 0, 0, {VK_RIGHT}},
    {"start", kStart, -1, -1, 0, 0, {VK_RETURN}},
    {"back", kBack, -1, -1, 0, 0, {VK_ESCAPE}},
    {"jump", 0, kA, -1, 0, 0, {VK_SPACE}},
    {"b", 0, kB, -1, 0, 0, {VK_LSHIFT, VK_RBUTTON}},
    {"attack", 0, kX, -1, 0, 0, {'F', VK_LBUTTON}},
    {"y", 0, kY, -1, 0, 0, {'R'}},
    {"black", 0, kBlack, -1, 0, 0, {'C'}},
    {"white", 0, kWhite, -1, 0, 0, {'V'}},
    {"left_trigger", 0, kLt, -1, 0, 0, {'Q'}},
    {"right_trigger", 0, kRt, -1, 0, 0, {'E'}},
    {"left_thumb", kLs, -1, -1, 0, 0, {VK_LCONTROL}},
    {"right_thumb", kRs, -1, -1, 0, 0, {VK_MBUTTON}},
    {"move_forward", 0, -1, 0, 0, 1, {'W'}},
    {"move_back", 0, -1, 0, 0, -1, {'S'}},
    {"move_left", 0, -1, 0, -1, 0, {'A'}},
    {"move_right", 0, -1, 0, 1, 0, {'D'}},
    {"camera_up", 0, -1, 1, 0, 1, {'I'}},
    {"camera_down", 0, -1, 1, 0, -1, {'K'}},
    {"camera_left", 0, -1, 1, -1, 0, {'J'}},
    {"camera_right", 0, -1, 1, 1, 0, {'L'}},
};

struct KeyName {
    const char* name;
    int vk;
};
constexpr KeyName kNamed[] = {
    {"Up", VK_UP},           {"Down", VK_DOWN},         {"Left", VK_LEFT},       {"Right", VK_RIGHT},
    {"Space", VK_SPACE},     {"Enter", VK_RETURN},      {"Escape", VK_ESCAPE},   {"Tab", VK_TAB},
    {"Backspace", VK_BACK},  {"LShift", VK_LSHIFT},     {"RShift", VK_RSHIFT},   {"LCtrl", VK_LCONTROL},
    {"RCtrl", VK_RCONTROL},  {"LAlt", VK_LMENU},        {"RAlt", VK_RMENU},      {"Mouse1", VK_LBUTTON},
    {"Mouse2", VK_RBUTTON},  {"Mouse3", VK_MBUTTON},    {"Mouse4", VK_XBUTTON1}, {"Mouse5", VK_XBUTTON2},
};

int KeyFromName(const std::string& n) {
    if (n.size() == 1 && ((n[0] >= 'A' && n[0] <= 'Z') || (n[0] >= '0' && n[0] <= '9'))) return n[0];
    if (n.size() == 1 && n[0] >= 'a' && n[0] <= 'z') return n[0] - 'a' + 'A';
    if ((n.size() == 2 || n.size() == 3) && n[0] == 'F') {
        int f = std::atoi(n.c_str() + 1);
        if (f >= 1 && f <= 12) return VK_F1 + f - 1;
    }
    for (const KeyName& k : kNamed)
        if (_stricmp(n.c_str(), k.name) == 0) return k.vk;
    return 0;
}

std::string NameOfKey(int vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, char(vk));
    if (vk >= VK_F1 && vk <= VK_F12) return "F" + std::to_string(vk - VK_F1 + 1);
    for (const KeyName& k : kNamed)
        if (k.vk == vk) return k.name;
    return {};
}

bool Focused() {
    DWORD pid = 0;
    HWND fg = GetForegroundWindow();
    return fg && GetWindowThreadProcessId(fg, &pid) && pid == GetCurrentProcessId();
}

bool Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

int16_t Clamp16(float v) { return int16_t(std::clamp(v, -32768.0f, 32767.0f)); }

}  // namespace

void LoadKeys(const void* table) {
    const auto* keys = (*static_cast<const toml::table*>(table))["keys"].as_table();
    if (!keys) return;
    for (Action& a : g_actions) {
        const toml::array* list = (*keys)[a.name].as_array();
        if (!list) continue;
        a.keys.clear();
        for (const auto& item : *list)
            if (auto s = item.value<std::string>())
                if (int vk = KeyFromName(*s)) a.keys.push_back(vk);
    }
}

void SaveKeys(void* table) {
    toml::table keys;
    for (const Action& a : g_actions) {
        toml::array list;
        for (int vk : a.keys)
            if (std::string n = NameOfKey(vk); !n.empty()) list.push_back(n);
        keys.insert(a.name, list);
    }
    static_cast<toml::table*>(table)->insert_or_assign("keys", keys);
}

// keyboard, mouse buttons and the mouse camera into the pad, while the game window has focus
void KeyboardPad(input::Pad& p) {
    if (!Focused()) return;
    float sx[2] = {}, sy[2] = {};
    for (const Action& a : g_actions) {
        bool down = false;
        for (int vk : a.keys) down |= Down(vk);
        if (!down) continue;
        p.buttons |= a.bit;
        if (a.analog >= 0) p.analog[a.analog] = 0xFF;
        if (a.stick >= 0) {
            sx[a.stick] += float(a.dx);
            sy[a.stick] += float(a.dy);
        }
    }
    if (sx[0] || sy[0]) {
        p.lx = Clamp16(sx[0] * 32767);
        p.ly = Clamp16(sy[0] * 32767);
    }
    if (sx[1] || sy[1]) {
        p.rx = Clamp16(sx[1] * 32767);
        p.ry = Clamp16(sy[1] * 32767);
    }
    if (!g_mouse_camera) return;
    // relative mouse: the cursor goes back to the window's centre each read, its move turns the camera
    HWND hwnd = static_cast<HWND>(app::Window());
    RECT rc;
    if (!hwnd || !GetClientRect(hwnd, &rc)) return;
    POINT centre{(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    ClientToScreen(hwnd, &centre);
    POINT at;
    if (!GetCursorPos(&at)) return;
    static bool first = true;
    if (!first) {
        constexpr float kPerPixel = 1600.0f;  // stick units per pixel moved between reads at 100%
        float k = kPerPixel * g_mouse_sensitivity / 100.0f;
        float mx = float(at.x - centre.x), my = float(at.y - centre.y);
        if (mx || my) {
            p.rx = Clamp16(float(p.rx) + mx * k);
            p.ry = Clamp16(float(p.ry) - my * k);
        }
    }
    first = false;
    SetCursorPos(centre.x, centre.y);
}

// the window thread: the cursor hidden over the window while the mouse turns the camera
bool KeyboardMessage(void*, uint32_t msg, uintptr_t, intptr_t lp, intptr_t* result) {
    if (msg == WM_SETCURSOR && g_mouse_camera && LOWORD(lp) == HTCLIENT && Focused()) {
        SetCursor(nullptr);
        *result = TRUE;
        return true;
    }
    return false;
}

// Keyboard controls and scripted test input, merged into controller port 0 (GameInfo::pad).
//
// Keyboard, while the game window has focus:
//   arrows      d-pad (menus)          W A S D     left stick (move)
//   I J K L     right stick (camera)   Enter       Start        Esc   Back
//   Space       A                      Left Shift  B            F     X        R   Y
//   Q / E       left / right trigger   C / V       black / white
//
// UNCHAINED_INPUT scripts presses for test runs, seconds after start: "30:start,32:a,33.5:down:400" (button, hold in ms,
// 150 by default). Buttons: up down left right start back a b x y black white lt rt ls rs.
#include "app/app.h"
#include "core/input.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>
#include <windows.h>

namespace {

enum Bit : uint16_t { kUp = 1, kDown = 2, kLeft = 4, kRight = 8, kStart = 16, kBack = 32, kLs = 64, kRs = 128 };
enum Analog { kA, kB, kX, kY, kBlack, kWhite, kLt, kRt };

void Press(input::Pad& p, const std::string& name) {
    static const struct {
        const char* name;
        uint16_t bit;
        int analog;
    } kMap[] = {{"up", kUp, -1},      {"down", kDown, -1},   {"left", kLeft, -1},   {"right", kRight, -1},
                {"start", kStart, -1}, {"back", kBack, -1},  {"ls", kLs, -1},       {"rs", kRs, -1},
                {"a", 0, kA},          {"b", 0, kB},         {"x", 0, kX},          {"y", 0, kY},
                {"black", 0, kBlack},  {"white", 0, kWhite}, {"lt", 0, kLt},        {"rt", 0, kRt}};
    for (const auto& m : kMap)
        if (name == m.name) {
            p.buttons |= m.bit;
            if (m.analog >= 0) p.analog[m.analog] = 0xFF;
        }
}

struct Scripted {
    double at;
    double hold;
    std::string button;
};

std::vector<Scripted> Script() {
    std::vector<Scripted> out;
    const char* s = std::getenv("UNCHAINED_INPUT");
    if (!s) return out;
    std::string all = s;
    size_t pos = 0;
    while (pos < all.size()) {
        size_t comma = all.find(',', pos);
        std::string item = all.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        size_t c1 = item.find(':'), c2 = c1 == std::string::npos ? c1 : item.find(':', c1 + 1);
        if (c1 != std::string::npos) {
            Scripted e{std::atof(item.substr(0, c1).c_str()), 0.15,
                       item.substr(c1 + 1, c2 == std::string::npos ? std::string::npos : c2 - c1 - 1)};
            if (c2 != std::string::npos) e.hold = std::atof(item.substr(c2 + 1).c_str()) / 1000.0;
            out.push_back(e);
        }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return out;
}

bool GameFocused() {
    DWORD pid = 0;
    HWND fg = GetForegroundWindow();
    return fg && GetWindowThreadProcessId(fg, &pid) && pid == GetCurrentProcessId();
}

bool Key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

// a stick from four keys, full deflection; y up
void Stick(int16_t& x, int16_t& y, int left, int right, int up, int down) {
    if (Key(left)) x = -32768;
    if (Key(right)) x = 32767;
    if (Key(up)) y = 32767;
    if (Key(down)) y = -32768;
}

}  // namespace

void UnchainedPad(int port, input::Pad& p) {
    if (port != 0) return;
    // port 0 always has a controller: the keyboard, as well as any real one
    p.connected = true;
    if (GameFocused()) {
        if (Key(VK_UP)) p.buttons |= kUp;
        if (Key(VK_DOWN)) p.buttons |= kDown;
        if (Key(VK_LEFT)) p.buttons |= kLeft;
        if (Key(VK_RIGHT)) p.buttons |= kRight;
        if (Key(VK_RETURN)) p.buttons |= kStart;
        if (Key(VK_ESCAPE)) p.buttons |= kBack;
        if (Key(VK_SPACE)) p.analog[kA] = 0xFF;
        if (Key(VK_LSHIFT)) p.analog[kB] = 0xFF;
        if (Key('F')) p.analog[kX] = 0xFF;
        if (Key('R')) p.analog[kY] = 0xFF;
        if (Key('C')) p.analog[kBlack] = 0xFF;
        if (Key('V')) p.analog[kWhite] = 0xFF;
        if (Key('Q')) p.analog[kLt] = 0xFF;
        if (Key('E')) p.analog[kRt] = 0xFF;
        Stick(p.lx, p.ly, 'A', 'D', 'W', 'S');
        Stick(p.rx, p.ry, 'J', 'L', 'I', 'K');
    }
    static const auto start = std::chrono::steady_clock::now();
    static const std::vector<Scripted> script = Script();
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    for (const auto& e : script)
        if (t >= e.at && t < e.at + e.hold) Press(p, e.button);
}

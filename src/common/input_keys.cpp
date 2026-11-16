// Keyboard controls and scripted test input, merged into controller port 0 (GameInfo::pad).
//
// Keyboard, while the game window has focus:
//   arrows      d-pad (menus)          W A S D     left stick (move)
//   I J K L     right stick (camera)   Enter       Start        Esc   Back
//   Space       A                      Left Shift  B            F     X        R   Y
//   Q / E       left / right trigger   C / V       black / white
//
// UNCHAINED_INPUT scripts presses for test runs, seconds after start: "30:start,32:a,33.5:down:400" (button, hold in ms,
// 150 by default). Buttons: up down left right start back a b x y black white lt rt ls rs; sticks lx- lx+ ly- ly+
// rx- rx+ ry- ry+ at full deflection. "@file" reads the script from that file.
//
// UNCHAINED_INPUT_RECORD=1 (or a file path) records what the player does in that same format, to input_record.txt
// beside the exe: each button press with its hold, and each stick past half way as its direction, timed from the
// first pad read as the script is. Replayed through UNCHAINED_INPUT=@input_record.txt (tools/tests: input_file), a
// test follows a real run instead of a blind script.
#include "app/app.h"
#include "core/input.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
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
    // sticks at full deflection: lx- lx+ ly- ly+ rx- rx+ ry- ry+ (y up)
    if (name.size() == 3 && (name[0] == 'l' || name[0] == 'r') && (name[1] == 'x' || name[1] == 'y')) {
        int16_t v = name[2] == '+' ? int16_t(32767) : int16_t(-32768);
        int16_t& axis = name[0] == 'l' ? (name[1] == 'x' ? p.lx : p.ly) : (name[1] == 'x' ? p.rx : p.ry);
        axis = v;
        return;
    }
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
    if (!all.empty() && all[0] == '@') {
        std::filesystem::path file = all.substr(1);
        if (file.is_relative()) file = app::ExeDir() / file;
        std::ifstream f(file);
        std::stringstream text;
        text << f.rdbuf();
        all = text.str();
        for (char& c : all)
            if (c == '\n' || c == '\r') c = ',';
    }
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

// the recorder: per input, when it went down; its release writes "<down>:<name>:<held ms>," at once, so a run that
// ends badly keeps what came before
class Recorder {
public:
    Recorder() {
        const char* v = std::getenv("UNCHAINED_INPUT_RECORD");
        if (!v || !*v) return;
        std::filesystem::path path = std::string(v) == "1" ? app::ExeDir() / "input_record.txt" : std::filesystem::path(v);
        file_ = _wfopen(path.c_str(), L"w");
    }
    void Update(const input::Pad& p, double t) {
        if (!file_) return;
        constexpr int16_t kHalf = 16384;
        constexpr uint8_t kAnalogDown = 0x40;
        static const char* const kButtons[] = {"up", "down", "left", "right", "start", "back", "ls", "rs"};
        static const char* const kAnalogs[] = {"a", "b", "x", "y", "black", "white", "lt", "rt"};
        for (int i = 0; i < 8; i++) Track(kButtons[i], (p.buttons >> i) & 1, t);
        for (int i = 0; i < 8; i++) Track(kAnalogs[i], p.analog[i] > kAnalogDown, t);
        Track("lx-", p.lx < -kHalf, t);
        Track("lx+", p.lx > kHalf, t);
        Track("ly-", p.ly < -kHalf, t);
        Track("ly+", p.ly > kHalf, t);
        Track("rx-", p.rx < -kHalf, t);
        Track("rx+", p.rx > kHalf, t);
        Track("ry-", p.ry < -kHalf, t);
        Track("ry+", p.ry > kHalf, t);
    }

private:
    void Track(const char* name, bool down, double t) {
        double& since = down_.try_emplace(name, -1.0).first->second;
        if (down && since < 0) since = t;
        if (!down && since >= 0) {
            fprintf(file_, "%.2f:%s:%.0f\n", since, name, (t - since) * 1000.0);
            fflush(file_);
            since = -1;
        }
    }
    FILE* file_ = nullptr;
    std::map<std::string, double> down_;  // when each went down, -1 while up
};

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
    static Recorder recorder;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    recorder.Update(p, t);  // the player's own input, before the script adds to it
    for (const auto& e : script)
        if (t >= e.at && t < e.at + e.hold) Press(p, e.button);
}

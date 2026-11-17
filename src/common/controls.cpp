// Player control settings on top of the pad the game reads (GameInfo::pad and ::rumble):
//   camera_speed    the right stick (the camera) scaled, percent: 100 as the game has it
//   invert_camera_x, invert_camera_y
//   vibration       rumble strength, percent: 0 off, 100 as the game asks
#include "app/app.h"
#include "core/input.h"

#include <algorithm>
#include <cstdint>

uint32_t g_camera_speed = 100;
bool g_invert_camera_x = false;
bool g_invert_camera_y = false;
uint32_t g_vibration = 100;

void UnchainedPad(int port, input::Pad& p);  // input_keys.cpp: keyboard and scripted test input

namespace {

constexpr uint32_t kMaxCameraSpeed = 300;

int16_t Axis(int16_t v, uint32_t percent, bool invert) {
    int32_t x = int32_t(v) * int32_t(percent) / 100;
    if (invert) x = -x;
    return int16_t(std::clamp<int32_t>(x, -32768, 32767));
}

}  // namespace

void PromptsPad(const input::Pad& p);  // prompts.cpp: the device in use

bool MenuPad(const input::Pad& p);      // menu.cpp: the in-game menu takes the controller while open
bool MenuSwallow(const input::Pad& p);  // and what is still held after it closed

void ControlsPad(int port, input::Pad& p) {
    bool menu = false;
    if (port == 0) {
        PromptsPad(p);  // the controller alone, before the keyboard joins it
        menu = MenuPad(p);
    }
    UnchainedPad(port, p);
    if (port == 0 && (menu || MenuSwallow(p))) {
        // the game gets nothing while the menu is open, nor the presses that closed it
        p = input::Pad{};
        p.connected = true;
        return;
    }
    uint32_t speed = std::min(g_camera_speed, kMaxCameraSpeed);
    p.rx = Axis(p.rx, speed, g_invert_camera_x);
    p.ry = Axis(p.ry, speed, g_invert_camera_y);
}

void ControlsRumble(int, uint16_t& left, uint16_t& right) {
    uint32_t v = std::min(g_vibration, 100u);
    left = uint16_t(uint32_t(left) * v / 100);
    right = uint16_t(uint32_t(right) * v / 100);
}

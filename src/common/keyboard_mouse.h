#pragma once

// Keyboard and mouse as a game controller (keyboard.cpp, input_keys.cpp): off in every version for now (user,
// 2026-11-17), the game is played with a controller. Off, keys and the mouse reach nothing of the game, the mouse
// camera never captures the cursor, the button prompts never show key caps, and the Configuration app and the in-game
// menu leave their settings out. The F-key shortcuts (F1 menu, F10 overlay, F12 screenshot) and scripted test input
// (UNCHAINED_INPUT) stay. true brings it all back.
inline constexpr bool kKeyboardMouse = false;

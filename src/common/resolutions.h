#pragma once
#include <cstdint>
#include <utility>
#include <vector>

// the primary display's resolutions (width, height in pixels), 640 wide and up, smallest first: the choices the
// Configuration app and the in-game menu offer beside "Desktop"
std::vector<std::pair<uint32_t, uint32_t>> DisplayResolutions();

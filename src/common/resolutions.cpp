#include "common/resolutions.h"

#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif

std::vector<std::pair<uint32_t, uint32_t>> DisplayResolutions() {
    std::vector<std::pair<uint32_t, uint32_t>> out;
#ifdef _WIN32
    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); i++) {
        std::pair<uint32_t, uint32_t> r{dm.dmPelsWidth, dm.dmPelsHeight};
        if (r.first >= 640 && std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r);
    }
#else
    // the primary display's fullscreen modes, in pixels
    int n = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &n);
    for (int i = 0; modes && i < n; i++) {
        float k = modes[i]->pixel_density > 0 ? modes[i]->pixel_density : 1.0f;
        std::pair<uint32_t, uint32_t> r{uint32_t(modes[i]->w * k + 0.5f), uint32_t(modes[i]->h * k + 0.5f)};
        if (r.first >= 640 && std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r);
    }
    SDL_free(modes);
#endif
    std::sort(out.begin(), out.end());
    return out;
}

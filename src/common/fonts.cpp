// The fonts the menu and the button prompts draw with: Windows' own (Bahnschrift, Consolas) from its fonts folder. On
// Linux a copy of the same file in fonts\ in the data folder or beside the executable, else the nearest font the
// system has: Liberation Sans Narrow for Bahnschrift (both narrow sans serifs) and, bold, for Impact; DejaVu Sans Mono
// for Consolas.
#include "common/fonts.h"

#include <cstdlib>

#include "app/app.h"
#include "app/win32_compat.h"

namespace fonts {

namespace {

bool Exists(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::is_regular_file(p, ec);
}

}  // namespace

std::filesystem::path File(const wchar_t* windows_file, bool bold) {
#ifdef _WIN32
    (void)bold;
    wchar_t windir[MAX_PATH];
    GetWindowsDirectoryW(windir, MAX_PATH);
    std::filesystem::path p = std::filesystem::path(windir) / L"Fonts" / windows_file;
    return Exists(p) ? p : std::filesystem::path();
#else
    for (const auto& dir : {app::DataDir(), app::ExeDir()}) {
        std::filesystem::path own = dir / "fonts" / windows_file;
        if (Exists(own)) return own;
    }
    std::wstring name = windows_file;
    bool mono = name.rfind(L"consola", 0) == 0;
    if (name.rfind(L"impact", 0) == 0) bold = true;  // Impact's heavy condensed words: the narrow bold
    const char* const narrow[] = {bold ? "/usr/share/fonts/truetype/liberation/LiberationSansNarrow-Bold.ttf"
                                       : "/usr/share/fonts/truetype/liberation/LiberationSansNarrow-Regular.ttf",
                                  bold ? "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
                                       : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
    const char* const fixed[] = {bold ? "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"
                                      : "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                                 bold ? "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf"
                                      : "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf"};
    for (const char* p : mono ? fixed : narrow)
        if (Exists(p)) return p;
    return {};
#endif
}

}  // namespace fonts

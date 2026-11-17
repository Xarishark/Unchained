// The fonts every window of ours draws with: the same files on Windows and Linux, shipped in fonts\ beside the
// executable (SIL Open Font License, fonts/OFL-*.txt). Each stands in for the Windows font the look was made with:
// Barlow Semi Condensed for Bahnschrift, Anton for Impact, Cascadia Mono for Consolas. A file of the same name in
// fonts\ in the data folder comes first; without the bundled ones (a build missing them) Windows' own, or on Linux
// the nearest the system has.
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

bool Starts(const std::wstring& name, const wchar_t* prefix) { return name.rfind(prefix, 0) == 0; }

// the bundled file for a Windows font name
const wchar_t* Bundled(const std::wstring& name, bool bold) {
    if (Starts(name, L"impact")) return L"Anton-Regular.ttf";
    if (Starts(name, L"consola")) return bold ? L"CascadiaMono-Bold.ttf" : L"CascadiaMono-Regular.ttf";
    return bold ? L"BarlowSemiCondensed-SemiBold.ttf" : L"BarlowSemiCondensed-Medium.ttf";
}

}  // namespace

std::filesystem::path File(const wchar_t* windows_file, bool bold) {
    std::wstring name = windows_file;
    std::filesystem::path bundled = Bundled(name, bold);
    for (const auto& dir : {app::DataDir(), app::ExeDir()}) {
        std::filesystem::path own = dir / "fonts" / bundled;
        if (Exists(own)) return own;
    }
#ifdef _WIN32
    wchar_t windir[MAX_PATH];
    GetWindowsDirectoryW(windir, MAX_PATH);
    std::filesystem::path p = std::filesystem::path(windir) / L"Fonts" / windows_file;
    return Exists(p) ? p : std::filesystem::path();
#else
    bool mono = Starts(name, L"consola");
    if (Starts(name, L"impact")) bold = true;  // Impact's heavy condensed words: the narrow bold
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

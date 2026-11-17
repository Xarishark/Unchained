#pragma once
#include <filesystem>

namespace fonts {

// the font standing in for a Windows font by its name there ("bahnschrift.ttf", "consola.ttf"): the bundled one in
// fonts\ beside the executable, the same on every system; empty when there is none. bold picks the bold face.
std::filesystem::path File(const wchar_t* windows_file, bool bold = false);

}  // namespace fonts

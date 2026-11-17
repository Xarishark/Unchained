#pragma once
#include <filesystem>

namespace fonts {

// a Windows font file by its name there ("bahnschrift.ttf", "consola.ttf"), or on Linux the nearest one; empty when
// there is none. bold picks the bold face where the font comes in separate files.
std::filesystem::path File(const wchar_t* windows_file, bool bold = false);

}  // namespace fonts

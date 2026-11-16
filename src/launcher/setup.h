#pragma once
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>

namespace launcher {

enum class Stage { Idle, Checking, Copying, Done, Failed };

struct Setup {
    std::mutex lock;  // guards everything but cancel
    Stage stage = Stage::Idle;
    std::string text;        // what is happening, or why it failed
    float progress = 0;      // 0..1 of the copy
    std::string found_sha1;  // the picked default.xbe's
    std::filesystem::path source;
    std::atomic<bool> cancel{false};
};

Setup& State();
// checks and copies the game from a disc image (.iso) or an extracted folder into assets beside the exe
void Start(const std::filesystem::path& source);
// assets beside the exe holds default.xbe or a disc image
bool GameFilesPresent();

}  // namespace launcher

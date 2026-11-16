// The game files setup the Configuration app runs (TODO.md items 2 and L2): the player picks their disc image (.iso)
// or extracted folder, its default.xbe is checked against the one release this build runs, and the files are copied
// to the assets folder beside the exe, where the game looks for them. Runs on its own thread; the screen reads Setup.
#include "launcher/setup.h"

#include "app/app.h"
#include "kernel/disc.h"

#include <algorithm>
#include <fstream>
#include <thread>

extern const char kImageSha1[];  // the xbe this executable was made from (CMake)

namespace launcher {

namespace fs = std::filesystem;

namespace {

Setup g_setup;

// where the game looks: assets beside the exe or in a folder above it (a build's output); beside the exe for a new one
fs::path AssetsDir() {
    std::error_code ec;
    if (app::DataDir() != app::ExeDir()) return app::DataDir() / "assets";  // a moved data folder holds them
    fs::path dir = app::ExeDir();
    constexpr int kLevels = 6;
    for (int i = 0; i < kLevels; i++) {
        if (fs::is_directory(dir / "assets", ec)) return dir / "assets";
        if (!dir.has_parent_path() || dir.parent_path() == dir) break;
        dir = dir.parent_path();
    }
    return app::ExeDir() / "assets";
}

void Set(Stage stage, const std::string& text, float progress = -1) {
    std::lock_guard<std::mutex> lock(g_setup.lock);
    g_setup.stage = stage;
    g_setup.text = text;
    if (progress >= 0) g_setup.progress = progress;
}

void Fail(const std::string& why) {
    std::lock_guard<std::mutex> lock(g_setup.lock);
    g_setup.stage = Stage::Failed;
    g_setup.text = why;
}

std::string U8(const fs::path& p) {
    auto s = p.u8string();
    return std::string(s.begin(), s.end());
}

// one file to another in 8 MB steps, done adding to *copied; false on a read or write error
bool CopyFile(const fs::path& from, const fs::path& to, uint64_t total, uint64_t& copied) {
    std::ifstream in(from, std::ios::binary);
    std::ofstream out(to, std::ios::binary | std::ios::trunc);
    if (!in || !out) return false;
    constexpr size_t kStep = 8u << 20;
    std::vector<char> buf(kStep);
    while (in) {
        in.read(buf.data(), std::streamsize(buf.size()));
        std::streamsize n = in.gcount();
        if (n <= 0) break;
        out.write(buf.data(), n);
        if (!out) return false;
        copied += uint64_t(n);
        {
            std::lock_guard<std::mutex> lock(g_setup.lock);
            g_setup.progress = total ? float(double(copied) / double(total)) : 1.0f;
        }
        if (g_setup.cancel) return false;
    }
    return !in.bad();
}

void Run(fs::path source) {
    std::error_code ec;
    bool iso = fs::is_regular_file(source, ec);
    Set(Stage::Checking, "Reading default.xbe from " + U8(source.filename()), 0);

    // the release check, before anything is copied
    std::vector<uint8_t> xbe;
    if (iso) {
        kernel::Disc disc;
        std::string error;
        if (!disc.Open(source, error)) return Fail(U8(source.filename()) + " is not an Xbox disc image: " + error);
        if (!disc.Load("default.xbe", xbe)) return Fail("The disc image has no default.xbe.");
    } else {
        std::ifstream f(source / "default.xbe", std::ios::binary);
        if (!f) return Fail("The folder has no default.xbe: pick the folder with the game's files.");
        xbe.assign(std::istreambuf_iterator<char>(f), {});
    }
    std::string sha = app::Sha1(xbe);
    {
        std::lock_guard<std::mutex> lock(g_setup.lock);
        g_setup.found_sha1 = sha;
    }
    if (sha != kImageSha1)
        return Fail("This is a different release of Whiplash. Unchained runs one Xbox release: its default.xbe SHA-1 is " +
                    std::string(kImageSha1) + "; this one's is " + sha + ".");

    // the copy: the image as it is (the game mounts it), or the folder's files
    fs::path assets = AssetsDir();
    fs::create_directories(assets, ec);
    if (ec) return Fail("Could not make " + U8(assets) + ": " + ec.message());
    uint64_t total = 0, copied = 0;
    std::vector<std::pair<fs::path, fs::path>> files;
    if (iso) {
        files.push_back({source, assets / source.filename()});
    } else {
        for (auto it = fs::recursive_directory_iterator(source, ec); !ec && it != fs::recursive_directory_iterator();
             it.increment(ec))
            if (it->is_regular_file(ec)) files.push_back({it->path(), assets / fs::relative(it->path(), source, ec)});
    }
    for (auto& [from, to] : files) total += fs::file_size(from, ec);
    if (fs::space(assets, ec).available < total)
        return Fail("Not enough free space for the game's files beside Unchained (" + std::to_string(total >> 20) +
                    " MB needed).");
    for (auto& [from, to] : files) {
        Set(Stage::Copying, "Copying " + U8(fs::relative(from, iso ? from.parent_path() : source, ec)));
        fs::create_directories(to.parent_path(), ec);
        if (!CopyFile(from, to, total, copied)) {
            fs::remove(to, ec);
            return Fail(g_setup.cancel ? "Stopped." : "Could not copy " + U8(from.filename()) + ".");
        }
    }
    Set(Stage::Done, "The game's files are ready.", 1);
}

}  // namespace

Setup& State() { return g_setup; }

void Start(const fs::path& source) {
    {
        std::lock_guard<std::mutex> lock(g_setup.lock);
        if (g_setup.stage == Stage::Checking || g_setup.stage == Stage::Copying) return;
        g_setup.stage = Stage::Checking;
        g_setup.progress = 0;
        g_setup.cancel = false;
        g_setup.source = source;
    }
    std::thread(Run, source).detach();
}

bool GameFilesPresent() {
    fs::path assets = AssetsDir();
    std::error_code ec;
    if (fs::is_regular_file(assets / "default.xbe", ec)) return true;
    for (auto& e : fs::directory_iterator(assets, ec)) {
        auto ext = e.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext == L".iso") return true;
    }
    return false;
}

}  // namespace launcher

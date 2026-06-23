#pragma once

#include "file_browser.hpp"
#include <filesystem>
#include <optional>
#include <vector>

namespace tracker {

// Result of running the file browser. The browser can either select a single
// file to play immediately, and/or accumulate a list of paths to add to the
// playlist queue.
struct FileBrowserResult {
    // A single file the user chose to play right now.
    std::optional<std::filesystem::path> selected_file;
    // Files and/or folders the user added to the queue with the add keys.
    std::vector<std::filesystem::path> queued_paths;

    bool has_selection() const {
        return selected_file.has_value() || !queued_paths.empty();
    }
};

FileBrowserResult run_file_browser(const std::filesystem::path& start_dir);

// Backwards-compatible helper that only returns a single selected file.
std::optional<std::filesystem::path> run_file_browser_ui(const std::filesystem::path& start_dir);

}

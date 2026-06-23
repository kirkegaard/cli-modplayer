#pragma once

#include <filesystem>
#include <vector>

namespace tracker {

// An ordered queue of module files with a current position.
// Advancing past the end wraps back to the start, and going before the
// start wraps to the end.
class Playlist {
public:
    Playlist() = default;

    // Add a single module file to the end of the queue. Returns true if the
    // file was added.
    bool add_file(const std::filesystem::path& path);

    // Recursively scan a folder and append every module file found Returns the
    // number of files added.
    std::size_t add_folder(const std::filesystem::path& folder, bool recursive = true);

    // Add a file or folder, dispatching to add_file / add_folder. Returns the
    // number of entries added.
    std::size_t add_path(const std::filesystem::path& path);

    bool empty() const { return entries_.empty(); }
    std::size_t size() const { return entries_.size(); }

    const std::vector<std::filesystem::path>& entries() const { return entries_; }
    std::size_t current_index() const { return current_index_; }

    // Returns the current track, or an empty path if the playlist is empty.
    std::filesystem::path current() const;

    // Advance to the next track, wrapping to the start at the end.
    // Returns the new current track.
    std::filesystem::path next();

    // Go to the previous track, wrapping to the end before the start.
    std::filesystem::path previous();

    // Jump directly to a specific index
    void set_current_index(std::size_t index);

    void clear();

private:
    std::vector<std::filesystem::path> entries_;
    std::size_t current_index_{0};
};

}

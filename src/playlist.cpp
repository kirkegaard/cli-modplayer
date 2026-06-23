#include "playlist.hpp"
#include "file_browser.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace tracker {

bool Playlist::add_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return false;
    }
    if (!FileBrowser::is_module_file(path)) {
        return false;
    }
    entries_.push_back(std::filesystem::absolute(path, ec));
    return true;
}

std::size_t Playlist::add_folder(const std::filesystem::path& folder, bool recursive) {
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) {
        return 0;
    }

    std::vector<std::filesystem::path> found;

    auto collect = [&](const std::filesystem::path& p) {
        if (FileBrowser::is_module_file(p)) {
            found.push_back(p);
        }
    };

    if (recursive) {
        for (auto it = std::filesystem::recursive_directory_iterator(
                 folder, std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            std::error_code file_ec;
            if (it->is_regular_file(file_ec)) {
                collect(it->path());
            }
        }
    } else {
        for (auto it = std::filesystem::directory_iterator(
                 folder, std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::directory_iterator(); it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            std::error_code file_ec;
            if (it->is_regular_file(file_ec)) {
                collect(it->path());
            }
        }
    }

    std::sort(found.begin(), found.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
        std::string as = a.string();
        std::string bs = b.string();
        std::transform(as.begin(), as.end(), as.begin(), ::tolower);
        std::transform(bs.begin(), bs.end(), bs.begin(), ::tolower);
        return as < bs;
    });

    std::size_t added = 0;
    for (const auto& p : found) {
        entries_.push_back(std::filesystem::absolute(p, ec));
        ++added;
    }
    return added;
}

std::size_t Playlist::add_path(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec)) {
        return add_folder(path);
    }
    return add_file(path) ? 1 : 0;
}

std::filesystem::path Playlist::current() const {
    if (entries_.empty()) {
        return {};
    }
    return entries_[current_index_];
}

std::filesystem::path Playlist::next() {
    if (entries_.empty()) {
        return {};
    }
    current_index_ = (current_index_ + 1) % entries_.size();
    return entries_[current_index_];
}

std::filesystem::path Playlist::previous() {
    if (entries_.empty()) {
        return {};
    }
    current_index_ = (current_index_ + entries_.size() - 1) % entries_.size();
    return entries_[current_index_];
}

void Playlist::set_current_index(std::size_t index) {
    if (entries_.empty()) {
        current_index_ = 0;
        return;
    }
    if (index >= entries_.size()) {
        index = entries_.size() - 1;
    }
    current_index_ = index;
}

void Playlist::clear() {
    entries_.clear();
    current_index_ = 0;
}

} // namespace tracker

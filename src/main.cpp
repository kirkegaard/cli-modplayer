#include "player.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "file_browser_ui.hpp"
#include "playlist.hpp"
#include "simple_ui.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    bool simple_mode = false;
    tracker::Playlist playlist;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--simple") {
            simple_mode = true;
        } else if (!arg.empty() && arg[0] != '-') {
            // A file or a folder: folders are expanded into their module files.
            std::filesystem::path path = arg;
            std::size_t added = playlist.add_path(path);
            if (added == 0) {
                std::cerr << "Skipping (no playable module files): " << path << std::endl;
            }
        }
    }

    // No tracks supplied on the command line: open the file browser to build a
    // queue. Enter plays a single file immediately, a/A queue files/folders.
    if (playlist.empty()) {
        auto result = tracker::run_file_browser(std::filesystem::current_path());
        if (result.selected_file) {
            playlist.add_path(*result.selected_file);
        }
        for (const auto& p : result.queued_paths) {
            playlist.add_path(p);
        }
        if (playlist.empty()) {
            std::cout << "No file selected. Exiting." << std::endl;
            return 0;
        }
    }

    tracker::Config config;
    bool continue_playing = true;

    while (continue_playing) {
        std::filesystem::path module_path = playlist.current();

        if (module_path.empty()) {
            std::cout << "Playlist is empty. Exiting." << std::endl;
            return 0;
        }

        if (!std::filesystem::exists(module_path)) {
            std::cerr << "File not found, skipping: " << module_path << std::endl;
            // Advance to next; if we wrapped back to the same single entry, bail.
            if (playlist.size() <= 1) {
                return 1;
            }
            playlist.next();
            continue;
        }

        try {
            tracker::Player player(module_path.string());
            player.set_volume(config.get_volume());
            player.start();

            tracker::UiAction action = tracker::UiAction::Quit;
            if (simple_mode) {
                tracker::SimpleUi simple_ui(player);
                action = simple_ui.run();
            } else {
                std::string module_name = module_path.stem().string();
                tracker::Ui ui(player, config, module_name);
                ui.set_queue_info(playlist.current_index(), playlist.size());
                action = ui.run();
            }

            player.stop();
            config.save();

            switch (action) {
                case tracker::UiAction::Quit:
                    continue_playing = false;
                    break;

                case tracker::UiAction::NextTrack:
                    // Advance to the next track, wrapping to the start at the end.
                    playlist.next();
                    continue_playing = true;
                    break;

                case tracker::UiAction::PrevTrack:
                    playlist.previous();
                    continue_playing = true;
                    break;

                case tracker::UiAction::OpenBrowser: {
                    auto browse_dir = module_path.parent_path();
                    if (browse_dir.empty()) {
                        browse_dir = std::filesystem::current_path();
                    }

                    auto result = tracker::run_file_browser(browse_dir);

                    bool changed = false;
                    // Queue any added files/folders onto the end of the playlist.
                    for (const auto& p : result.queued_paths) {
                        if (playlist.add_path(p) > 0) {
                            changed = true;
                        }
                    }
                    // A single Enter-selected file plays immediately: append it
                    // and jump to it.
                    if (result.selected_file) {
                        std::size_t before = playlist.size();
                        if (playlist.add_path(*result.selected_file) > 0) {
                            playlist.set_current_index(before);
                            changed = true;
                        }
                    }

                    // Whether or not anything was added, stay in the loop and
                    // (re)play the current queue position. If a file was
                    // selected, current_index already points at it.
                    (void)changed;
                    continue_playing = true;
                    break;
                }
            }

        } catch (const std::exception &ex) {
            std::cerr << "Fatal error: " << ex.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "Unknown error occurred." << std::endl;
            return 1;
        }
    }

    return 0;
}

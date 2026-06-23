#include "player.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "file_browser_ui.hpp"
#include "simple_ui.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    std::filesystem::path module_path;
    bool simple_mode = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--simple") simple_mode = true;
        else if (arg[0] != '-') module_path = arg;
    }
    
    if (module_path.empty()) {
        auto selected = tracker::run_file_browser_ui(std::filesystem::current_path());
        if (!selected) {
            std::cout << "No file selected. Exiting." << std::endl;
            return 0;
        }
        module_path = *selected;
    }
    
    tracker::Config config;
    bool continue_playing = true;
    
    while (continue_playing) {
        if (!std::filesystem::exists(module_path)) {
            std::cerr << "File not found: " << module_path << std::endl;
            return 1;
        }
        
        try {
            tracker::Player player(module_path.string());
            player.set_volume(config.get_volume());
            player.start();
            
            bool open_new_file = false;
            if (simple_mode) {
                tracker::SimpleUi simple_ui(player);
                simple_ui.run();
                open_new_file = false;
            } else {
                std::string module_name = module_path.stem().string();
                tracker::Ui ui(player, config, module_name);
                open_new_file = ui.run();
            }
            
            player.stop();
            config.save();
            
            if (open_new_file) {
                // Get the directory of the current file, or use current directory if empty
                auto browse_dir = module_path.parent_path();
                if (browse_dir.empty()) {
                    browse_dir = std::filesystem::current_path();
                }
                
                auto selected = tracker::run_file_browser_ui(browse_dir);
                if (selected) {
                    module_path = *selected;
                    continue_playing = true;
                } else {
                    continue_playing = false;
                }
            } else {
                continue_playing = false;
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

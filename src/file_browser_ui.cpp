#include "file_browser_ui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

namespace tracker {

namespace {

ftxui::Color kAccent = ftxui::Color::RGB(129, 200, 190);
ftxui::Color kBackground = ftxui::Color::RGB(16, 18, 26);
ftxui::Color kPanel = ftxui::Color::RGB(26, 28, 38);
ftxui::Color kPanelAlt = ftxui::Color::RGB(32, 34, 46);
ftxui::Color kBorder = ftxui::Color::RGB(118, 92, 199);
ftxui::Color kText = ftxui::Color::RGB(230, 230, 230);
ftxui::Color kTextDim = ftxui::Color::RGB(160, 164, 182);
ftxui::Color kSuccess = ftxui::Color::RGB(124, 200, 146);
ftxui::Color kWarning = ftxui::Color::RGB(230, 196, 84);

std::string format_file_size(std::size_t bytes) {
    std::ostringstream oss;
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else {
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    return oss.str();
}

}

FileBrowserResult run_file_browser(const std::filesystem::path& start_dir) {
    using namespace ftxui;
    
    FileBrowser browser(start_dir);
    FileBrowserResult result;
    bool quit = false;
    int queued_count = 0;
    std::string status_message;
    
    auto screen = ScreenInteractive::Fullscreen();
    
    auto component = Renderer([&] {
        Elements file_list;
        
        const auto& entries = browser.entries();
        const std::size_t selected = browser.selected_index();
        
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = entries[i];
            bool is_selected = (i == selected);
            
            std::string prefix = entry.is_directory ? "📁 " : "🎵 ";
            std::string name = entry.display_name;
            std::string size_str = entry.is_directory ? "<DIR>" : format_file_size(entry.size);
            
            auto name_element = text(prefix + name);
            auto size_element = text(size_str) | align_right;
            
            auto line = hbox({
                name_element | flex,
                text("  "),
                size_element | size(WIDTH, EQUAL, 12)
            });
            
            if (is_selected) {
                line = line | bgcolor(kPanelAlt) | color(kAccent) | bold | focus;
            } else if (entry.is_directory) {
                line = line | color(kSuccess);
            } else {
                line = line | color(kText);
            }
            
            file_list.push_back(line);
        }
        
        if (file_list.empty()) {
            file_list.push_back(text("No module files found") | color(kTextDim) | dim | center);
        }
        
        auto current_path_display = text("📂 " + browser.current_path().string()) | 
                                   color(kAccent) | bold;
        
        auto help_text = hbox({
            text("↑↓: Navigate  ") | color(kTextDim),
            text("Enter: Play/Open  ") | color(kTextDim),
            text("a: Queue  ") | color(kTextDim),
            text("A: Queue folder  ") | color(kTextDim),
            text("Backspace: Parent  ") | color(kTextDim),
            text("Q: Done") | color(kWarning)
        }) | center;

        std::string queue_label = "Queue: " + std::to_string(queued_count) + " track" +
                                  (queued_count == 1 ? "" : "s");
        Element status_line;
        if (!status_message.empty()) {
            status_line = hbox({
                text(queue_label) | color(kSuccess) | bold,
                text("   "),
                text(status_message) | color(kTextDim)
            });
        } else {
            status_line = text(queue_label) | color(kSuccess) | bold;
        }
        
        auto content = vbox({
            text("═══ cli-modplayer v1.3.0 ═══") | bold | color(kAccent) | center,
            separator(),
            current_path_display,
            status_line,
            separator(),
            vbox(file_list) | yflex | bgcolor(kPanel) | vscroll_indicator | yframe | focus,
            separator(),
            help_text
        }) | border | color(kBorder) | bgcolor(kBackground);
        
        if (browser.has_error()) {
            auto error_box = vbox({
                text("Error:") | bold | color(kWarning),
                text(browser.error_message()) | color(kText)
            }) | border | bgcolor(kPanel) | color(kWarning) | center;
            
            return dbox({content, error_box});
        }
        
        return content;
    });
    
    component = CatchEvent(component, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q') || 
            event == Event::Escape) {
            // If nothing was queued, treat as a cancel.
            if (result.queued_paths.empty()) {
                quit = true;
            }
            screen.Exit();
            return true;
        }
        
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            browser.select_next();
            return true;
        }
        
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            browser.select_previous();
            return true;
        }
        
        if (event == Event::Home) {
            browser.select_first();
            return true;
        }
        
        if (event == Event::End) {
            browser.select_last();
            return true;
        }
        
        if (event == Event::PageDown) {
            for (int i = 0; i < 10; ++i) browser.select_next();
            return true;
        }
        
        if (event == Event::PageUp) {
            for (int i = 0; i < 10; ++i) browser.select_previous();
            return true;
        }
        
        if (event == Event::Backspace) {
            browser.navigate_up();
            return true;
        }

        // Add the selected entry to the queue.
        if (event == Event::Character('a')) {
            const auto& entries = browser.entries();
            const std::size_t selected = browser.selected_index();
            if (selected < entries.size()) {
                const auto& entry = entries[selected];
                if (entry.display_name == "..") {
                    status_message = "Cannot queue parent entry";
                } else {
                    result.queued_paths.push_back(entry.path);
                    ++queued_count;
                    if (entry.is_directory) {
                        status_message = "Queued folder: " + entry.display_name;
                    } else {
                        status_message = "Queued: " + entry.display_name;
                    }
                    browser.select_next();
                }
            }
            return true;
        }

        // Add the entire current folder to the queue.
        if (event == Event::Character('A')) {
            result.queued_paths.push_back(browser.current_path());
            ++queued_count;
            status_message = "Queued current folder";
            return true;
        }
        
        if (event == Event::Return) {
            const auto& entries = browser.entries();
            const std::size_t selected = browser.selected_index();
            
            if (selected < entries.size()) {
                const auto& entry = entries[selected];
                
                if (entry.is_directory) {
                    browser.navigate_into(selected);
                } else {
                    result.selected_file = entry.path;
                    screen.Exit();
                }
            }
            return true;
        }
        
        return false;
    });
    
    screen.Loop(component);
    
    if (quit) {
        return FileBrowserResult{};
    }
    
    return result;
}

std::optional<std::filesystem::path> run_file_browser_ui(const std::filesystem::path& start_dir) {
    auto result = run_file_browser(start_dir);
    if (result.selected_file) {
        return result.selected_file;
    }
    if (!result.queued_paths.empty()) {
        // Caller only wants a single file; ignore queued paths here.
        return std::nullopt;
    }
    return std::nullopt;
}

} 

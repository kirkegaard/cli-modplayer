#pragma once
#include "player.hpp"
#include "ui.hpp"
#include <atomic>

namespace tracker {

class SimpleUi {
public:
    SimpleUi(Player& player);
    UiAction run();
private:
    Player& player_;
    std::atomic<bool> running_{true};
};

}

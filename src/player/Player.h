#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <memory>

struct Module;
struct Player {

    Player(const std::shared_ptr<Module>& mod);

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
};

#endif
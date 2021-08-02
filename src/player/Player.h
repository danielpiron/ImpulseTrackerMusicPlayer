#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <memory>

struct Module;
struct PatternEntry;
struct Player {

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();
    void process_tick();

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    size_t current_row;
};

#endif
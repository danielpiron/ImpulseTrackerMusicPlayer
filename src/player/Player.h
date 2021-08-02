#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <memory>
#include <player/PatternEntry.h>

struct Module;
struct Player {

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();

    void process_global_command(const PatternEntry::Effect& effect);
    void process_tick();

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    size_t current_row;
    size_t current_order;
};

#endif
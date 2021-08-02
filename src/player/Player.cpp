#include "Player.h"
#include "Module.h"

Player::Player(const std::shared_ptr<Module>& mod)
    : module(std::const_pointer_cast<const Module>(mod)),
      speed(mod->initial_speed), tempo(mod->initial_tempo), current_row(0)
{
}

const std::vector<PatternEntry>& Player::next_row()
{
    static std::vector<PatternEntry> row;

    row.clear();
    row.push_back(module->patterns[0].channel(0).row(current_row));

    current_row++;
    return row;
}

void Player::process_tick()
{
    for (const auto& entry : next_row()) {
        if (entry._effect._comm == PatternEntry::Command::set_speed) {
            speed = entry._effect._data;
        }
    }
}
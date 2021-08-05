#include "Player.h"
#include "Module.h"

Player::Player(const std::shared_ptr<Module>& mod)
    : module(std::const_pointer_cast<const Module>(mod)),
      speed(mod->initial_speed), tempo(mod->initial_tempo), current_row(0),
      current_order(0)
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

void Player::process_global_command(const PatternEntry::Effect& effect)
{
    switch (effect.comm) {
    case PatternEntry::Command::set_speed:
        speed = effect.data;
        break;
    case PatternEntry::Command::jump_to_order:
        current_order = effect.data;
        current_row = 0;
        break;
    case PatternEntry::Command::break_to_row:
        current_order++;
        current_row = effect.data;
        break;
    case PatternEntry::Command::set_tempo:
        tempo = effect.data;
        break;
    default:
        break;
    }
}

const std::vector<Player::Channel::Event>& Player::process_tick()
{
    static std::vector<Player::Channel::Event> channelEvents;
    channelEvents.clear();

    for (const auto& entry : next_row()) {
        process_global_command(entry._effect);

        if (entry._inst) {
            channelEvents.push_back(Player::Channel::Event{
                1, Player::Channel::Event::SetSample{entry._inst}});
        }

        switch (entry._volume_effect.comm) {
        case PatternEntry::Command::set_volume:
            channelEvents.push_back(Player::Channel::Event{
                1, Player::Channel::Event::SetVolume{
                       static_cast<float>(entry._volume_effect.data) / 64.0f}});
            break;
        default:
            break;
        }
    }
    return channelEvents;
}
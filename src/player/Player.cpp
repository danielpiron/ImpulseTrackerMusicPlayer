#include "Player.h"
#include "Module.h"

#include <iostream>

std::ostream& operator<<(std::ostream& os, const Player::Channel::Event& event)
{
    os << "Channel" << event.channel << " receives " << event.action;
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const Player::Channel::Event::Action& action)
{

    struct ActionPrinter {
        void
        operator()(const Player::Channel::Event::SetFrequency& setFreq) const
        {
            os << "set frequency to" << setFreq.frequency;
        }
        void operator()(const Player::Channel::Event::SetSample& setSamp) const
        {
            os << "set sample to" << setSamp.sampleIndex;
        }
        void operator()(const Player::Channel::Event::SetVolume& setVol) const
        {
            os << "set volume to" << setVol.volume;
        }
        std::ostream& os;
    };
    std::visit(ActionPrinter{os}, action);

    return os;
}

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

static const int note_periods[] = {1712, 1616, 1524, 1440, 1356, 1280,
                                   1208, 1140, 1076, 1016, 960,  907};

const std::vector<Player::Channel::Event>& Player::process_tick()
{
    static std::vector<Player::Channel::Event> channelEvents;
    channelEvents.clear();

    for (const auto& entry : next_row()) {
        process_global_command(entry._effect);

        if (!entry._note.is_empty()) {
            auto note_st3period =
                ((8363 * 32 * note_periods[entry._note.index()]) >>
                 entry._note.octave()) /
                8363;
            auto playback_frequency = 14317456 / note_st3period;
            channelEvents.push_back(Player::Channel::Event{
                1, Player::Channel::Event::SetFrequency{
                       static_cast<float>(playback_frequency)}});
        }
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
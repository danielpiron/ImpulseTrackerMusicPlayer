#include "Player.h"
#include "Mixer.h"
#include "Module.h"

#include <iostream>

std::ostream& operator<<(std::ostream& os, const Player::Channel::Event& event)
{
    os << "Channel " << event.channel << " " << event.action;
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const Player::Channel::Event::Action& action)
{

    struct ActionPrinter {
        void operator()(const Player::Channel::Event::NoteOn&) const
        {
            os << "note on";
        }
        void
        operator()(const Player::Channel::Event::SetFrequency& setFreq) const
        {
            os << "set frequency to " << setFreq.frequency;
        }
        void operator()(const Player::Channel::Event::SetSample& setSamp) const
        {
            os << "set sample to " << setSamp.sampleIndex;
        }
        void operator()(const Player::Channel::Event::SetVolume& setVol) const
        {
            os << "set volume to " << setVol.volume;
        }
        std::ostream& os;
    };
    std::visit(ActionPrinter{os}, action);

    return os;
}

Player::Player(const std::shared_ptr<Module>& mod)
    : module(std::const_pointer_cast<const Module>(mod)),
      speed(mod->initial_speed), tempo(mod->initial_tempo), current_row(0),
      current_order(0), channels(1), _mixer(16)

{
    _mixer.attach_handler(this);
}

void Player::onAttachment(Mixer&) {}

void Player::onTick(Mixer& audio)
{

    struct EventActionInterpretter {
        EventActionInterpretter(const Player& player, Mixer& mixer,
                                size_t channel)
            : player(player), mmixer(mixer), channel(channel)
        {
        }

        void operator()(const Player::Channel::Event::NoteOn&)
        {
            //    mixer.channel(channel).play();
        }
        void operator()(const Player::Channel::Event::SetFrequency& setFreq)
        {
            mmixer.channel(channel).set_frequency(setFreq.frequency);
        }
        void operator()(const Player::Channel::Event::SetSample&) {}
        void operator()(const Player::Channel::Event::SetVolume&) {}
        const Player& player;
        Mixer& mmixer;
        size_t channel;
    };

    EventActionInterpretter ea{*this, audio, 0};
    for (const auto& event : process_tick()) {
        ea.channel = static_cast<size_t>(event.channel);
        std::visit(ea, event.action);
    }
}

const std::vector<PatternEntry>& Player::next_row()
{
    static std::vector<PatternEntry> row;

    const auto& current_pattern =
        module->patterns[module->patternOrder[current_order]];

    row.clear();
    row.push_back(current_pattern.channel(0).row(current_row));

    if (++current_row == current_pattern.row_count()) {
        current_order++;
        // TODO: End of order value 255 needs a name
        if (module->patternOrder[current_order] == 255) {
            current_order = 0;
        }
        current_row = 0;
    }

    return row;
}

void Player::render_audio(float* buffer, int framesToRender)
{
    _mixer.render(buffer, static_cast<size_t>(framesToRender));
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

        int channel_index = 0;
        auto& channel = channels[static_cast<size_t>(channel_index)];
        if (!entry._note.is_empty()) {
            channel.last_note = entry._note;
        }
        if (entry._inst) {
            channel.last_inst = entry._inst;
        }

        if (channel.last_note.is_playable() && channel.last_inst) {
            auto note_st3period =
                ((8363 * 32 * note_periods[channel.last_note.index()]) >>
                 channel.last_note.octave()) /
                static_cast<int>(
                    module->samples[channel.last_inst - 1].playbackRate());
            auto playback_frequency = 14317456 / note_st3period;
            channelEvents.push_back(Player::Channel::Event{
                channel_index, Player::Channel::Event::SetFrequency{
                                   static_cast<float>(playback_frequency)}});
            channelEvents.push_back(Player::Channel::Event{
                channel_index,
                Player::Channel::Event::SetSample{
                    static_cast<PatternEntry::Inst>(channel.last_inst)}});
            channelEvents.push_back(Player::Channel::Event{
                channel_index, Player::Channel::Event::NoteOn{}});
        }

        switch (entry._volume_effect.comm) {
        case PatternEntry::Command::set_volume:
            channelEvents.push_back(Player::Channel::Event{
                channel_index,
                Player::Channel::Event::SetVolume{
                    static_cast<float>(entry._volume_effect.data) / 64.0f}});
            break;
        default:
            break;
        }
    }
    return channelEvents;
}
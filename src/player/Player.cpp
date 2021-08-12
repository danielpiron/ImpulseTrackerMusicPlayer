#include "Player.h"
#include "Mixer.h"
#include "Module.h"

#include <iostream>

Player::Player(const std::shared_ptr<Module>& mod)
    : module(std::const_pointer_cast<const Module>(mod)),
      speed(mod->initial_speed), tempo(mod->initial_tempo), tick_counter(1),
      current_row(0), current_order(0), channels(32), _mixer(44100, 16)
{
    _mixer.attach_handler(this);
}

void Player::onAttachment(Mixer& audio)
{
    audio.set_samples_per_tick(
        static_cast<size_t>(2.5f * static_cast<float>(audio.sampling_rate()) /
                            static_cast<float>(tempo)));
}

void Player::onTick(Mixer& audio)
{
    for (const auto& event : process_tick()) {
        audio.process_event(event);
    }
}

const std::vector<PatternEntry>& Player::next_row()
{
    static std::vector<PatternEntry> row;

    const auto& current_pattern =
        module->patterns[module->patternOrder[current_order]];

    row.clear();
    for (size_t i = 0; i < current_pattern.channel_count(); ++i) {
        row.push_back(current_pattern.channel(i).row(current_row));
    }

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

const std::vector<Mixer::Event>& Player::process_tick()
{
    static const int note_periods[] = {1712, 1616, 1524, 1440, 1356, 1280,
                                       1208, 1140, 1076, 1016, 960,  907};

    mixer_events.clear();
    if (--tick_counter > 0) {
        return mixer_events;
    }

    int channel_index = 0;
    tick_counter = speed;
    for (const auto& entry : next_row()) {

        if (channel_index < 8)
            std::cout << entry << " ";

        process_global_command(entry._effect);

        auto& channel = channels[static_cast<size_t>(channel_index)];
        channel.last_volume = channel.volume;

        bool candidate_note = false;
        if (!entry._note.is_empty()) {
            channel.last_note = entry._note;
            candidate_note = true;
        }
        if (entry._inst) {
            channel.last_inst = entry._inst;
            candidate_note = true;
        }

        if (candidate_note && channel.last_note.is_playable() &&
            channel.last_inst) {
            auto note_st3period =
                ((8363 * 32 * note_periods[channel.last_note.index()]) >>
                 channel.last_note.octave()) /
                static_cast<int>(
                    module->samples[static_cast<size_t>(channel.last_inst - 1)]
                        .sample.playbackRate());
            auto playback_frequency = 14317456 / note_st3period;

            channel.volume =
                module->samples[static_cast<size_t>(channel.last_inst - 1)]
                    .default_volume;
            mixer_events.push_back(
                {static_cast<size_t>(channel_index),
                 ::Channel::Event::SetNoteOn{
                     static_cast<float>(playback_frequency),
                     &(module
                           ->samples[static_cast<size_t>(channel.last_inst - 1)]
                           .sample)}});
        }

        switch (entry._volume_effect.comm) {
        case PatternEntry::Command::set_volume:
            channel.volume = static_cast<int8_t>(entry._volume_effect.data);
            break;
        default:
            break;
        }

        if (channel.volume != channel.last_volume) {
            mixer_events.push_back(
                {static_cast<size_t>(channel_index),
                 ::Channel::Event::SetVolume{
                     static_cast<float>(channel.volume) / 64.0f}});
        }
        channel_index++;
    }
    std::cout << std::endl;
    return mixer_events;
}
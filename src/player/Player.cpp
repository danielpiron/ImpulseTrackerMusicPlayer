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
        if (module->patternOrder[current_order] == 255) {
            current_order = 0;
        }
        current_row = effect.data;
        break;
    case PatternEntry::Command::set_tempo:
        tempo = effect.data;
        _mixer.set_samples_per_tick(static_cast<size_t>(
            2.5f * static_cast<float>(_mixer.sampling_rate()) /
            static_cast<float>(tempo)));
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
        for (size_t i = 0; i < channels.size(); ++i) {
            auto& channel = channels[i];

            // Volume Slide
            channel.last_volume = channel.volume;
            channel.volume += channel.volume_slide;

            if (channel.volume < 0)
                channel.volume = 0;
            if (channel.volume > 64)
                channel.volume = 64;

            if (channel.volume != channel.last_volume) {
                mixer_events.push_back(
                    {i, ::Channel::Event::SetVolume{
                            static_cast<float>(channel.volume) / 64.0f}});
            }

            // Pitch Slide
            channel.last_period = channel.period;
            channel.period += channel.pitch_slide;
            if ((channel.pitch_slide > 0 &&
                 channel.period > channel.target_period) ||
                (channel.pitch_slide < 0 &&
                 channel.period < channel.target_period)) {
                channel.period = channel.target_period;
                channel.pitch_slide = 0;
            }

            if (channel.period != channel.last_period) {
                mixer_events.push_back(
                    {i, ::Channel::Event::SetFrequency{
                            static_cast<float>(14317456 / channel.period)}});
            }
        }
        return mixer_events;
    }

    int channel_index = 0;
    for (const auto& entry : next_row()) {
        if (channel_index < 8)
            std::cout << entry << "|";

        process_global_command(entry._effect);

        auto& channel = channels[static_cast<size_t>(channel_index)];
        channel.last_volume = channel.volume;

        if (entry._effect.comm == PatternEntry::Command::volume_slide) {
            auto data = (entry._effect.data) ? entry._effect.data
                                             : channel.volume_slide_memory;

            channel.volume_slide_memory = data;
            if ((data & 0x0F) == 0) {
                channel.volume_slide = data >> 4;
            } else {
                channel.volume_slide = -(data & 0x0F);
            }
        } else {
            channel.volume_slide = 0;
        }

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
            channel.volume =
                module->samples[static_cast<size_t>(channel.last_inst - 1)]
                    .default_volume;

            if (entry._effect.comm != PatternEntry::Command::portamento) {
                channel.period = note_st3period;
                auto playback_frequency = 14317456 / note_st3period;
                mixer_events.push_back(
                    {static_cast<size_t>(channel_index),
                     ::Channel::Event::SetNoteOn{
                         static_cast<float>(playback_frequency),
                         &(module
                               ->samples[static_cast<size_t>(channel.last_inst -
                                                             1)]
                               .sample)}});
            }

            if (entry._effect.comm == PatternEntry::Command::pitch_slide_down ||
                entry._effect.comm == PatternEntry::Command::pitch_slide_up ||
                entry._effect.comm == PatternEntry::Command::portamento) {
                auto data = entry._effect.data ? entry._effect.data
                                               : channel.pitch_slide_memory;
                channel.pitch_slide_memory = data;
                channel.pitch_slide = data * 4;
                if (entry._effect.comm == PatternEntry::Command::portamento) {
                    channel.target_period = note_st3period;
                } else if (entry._effect.comm ==
                           PatternEntry::Command::pitch_slide_up) {
                    channel.target_period = 1; // calculate for lowest
                } else if (entry._effect.comm ==
                           PatternEntry::Command::pitch_slide_down) {
                    channel.target_period = 100000; // calculate for highest
                }
                if (channel.target_period < channel.period) {
                    channel.pitch_slide = -channel.pitch_slide;
                }
            }
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
    tick_counter = speed;
    std::cout << std::endl;
    return mixer_events;
}
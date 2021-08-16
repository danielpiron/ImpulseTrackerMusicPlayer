#include "Player.h"
#include "Mixer.h"
#include "Module.h"

#include <algorithm>
#include <iostream>

Player::Player(const std::shared_ptr<Module>& mod)
    : module(std::const_pointer_cast<const Module>(mod)),
      speed(mod->initial_speed),
      tempo(mod->initial_tempo),
      tick_counter(1),
      break_row(0),
      current_row(0),
      current_order(0),
      process_row(0),
      channels(32),
      _mixer(44100, 16)
{
    _mixer.attach_handler(this);
}

void Player::onAttachment(Mixer& audio)
{
    audio.set_samples_per_tick(static_cast<size_t>(2.5f * audio.sampling_rate() / tempo));
}

void Player::onTick(Mixer& audio)
{
    for (const auto& event : process_tick()) {
        audio.process_event(event);
    }
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
        tick_counter = speed;
        break;
    case PatternEntry::Command::jump_to_order:
        current_order = effect.data - 1;
        process_row = 0xFFFE;
        break;
    case PatternEntry::Command::break_to_row:
        process_row = 0xFFFE;
        break_row = effect.data;
        break;
    case PatternEntry::Command::set_tempo:
        tempo = effect.data;
        break;
    default:
        break;
    }
}

void Player::process_initial_tick(Player::Channel& channel, const PatternEntry& entry)
{
    static const int note_periods[] = {1712, 1616, 1524, 1440, 1356, 1280,
                                       1208, 1140, 1076, 1016, 960,  907};

    bool candidate_note = false;
    if (!entry._note.is_empty()) {
        channel.last_note = entry._note;
        candidate_note = true;
    }
    if (entry._inst) {
        channel.last_inst = entry._inst;
        candidate_note = true;
    }

    if (candidate_note && channel.last_note.is_playable() && channel.last_inst) {
        channel.note_on = true;
        channel.period =
            ((8363 * 32 * note_periods[channel.last_note.index()]) >> channel.last_note.octave()) /
            static_cast<int>(module->samples[channel.last_inst - 1].sample.playbackRate());

        channel.volume = module->samples[channel.last_inst - 1].default_volume;
    }

    switch (entry._volume_effect.comm) {
    case PatternEntry::Command::set_volume:
        channel.volume = static_cast<int8_t>(entry._volume_effect.data);
        break;
    default:
        break;
    }

    channel.effects.volume_slide = 0;
    channel.effects.pitch_slide = 0;
    if (entry._effect.comm == PatternEntry::Command::volume_slide) {
        auto data = entry._effect.data ? entry._effect.data : channel.effects_memory.volume_slide;
        channel.effects_memory.volume_slide = data;
        if ((data & 0xF0) == 0xF0) {
            // Fine slide down
            channel.volume -= data & 0x0F;
        } else if ((data & 0x0F) == 0x0F) {
            // Fine slide up
            channel.volume += data >> 4;
        } else if ((data & 0x0F) && (data & 0xF0) == 0) {
            // Slide down
            channel.effects.volume_slide = -static_cast<int8_t>(data & 0x0F);
        } else if ((data & 0xF0) && (data & 0x0F) == 0) {
            // Slide up
            channel.effects.volume_slide = static_cast<int8_t>((data >> 4));
        }
    } else if (entry._effect.comm == PatternEntry::Command::pitch_slide_down) {
        auto data = entry._effect.data ? entry._effect.data : channel.effects_memory.pitch_slide;
        channel.effects_memory.pitch_slide = data;
        if ((data & 0xF0) == 0xE0) {
            channel.period += (data & 0x0F);
        } else if ((data & 0xF0) == 0xF0) {
            channel.period += (data & 0x0F) * 4;
        } else {
            channel.effects.pitch_slide = static_cast<int8_t>(data * 4);
        }
    }
}

static void update_effects(Player::Channel& channel)
{
    channel.volume += channel.effects.volume_slide;
    channel.period += channel.effects.pitch_slide;
}

const std::vector<Mixer::Event>& Player::process_tick()
{
    mixer_events.clear();

    bool initial_tick = --tick_counter == 0;

    const auto& current_pattern = module->patterns[module->patternOrder[current_order]];
    for (size_t channel_index = 0; channel_index < current_pattern.channel_count();
         ++channel_index) {

        auto& channel = channels[static_cast<size_t>(channel_index)];

        auto last_volume = channel.volume;
        auto last_period = channel.period;

        if (initial_tick) {
            const auto& entry = current_pattern.channel(channel_index).row(current_row);
            process_global_command(entry._effect);
            process_initial_tick(channel, entry);
        } else {
            update_effects(channel);
        }

        if (channel.note_on || channel.period != last_period) {
            auto playback_frequency = 14317456 / channel.period;
            if (channel.note_on) {
                mixer_events.push_back({static_cast<size_t>(channel_index),
                                        ::Channel::Event::SetNoteOn{
                                            static_cast<float>(playback_frequency),
                                            &(module->samples[channel.last_inst - 1].sample)}});
                channel.note_on = false;
            } else {
                mixer_events.push_back(
                    {static_cast<size_t>(channel_index),
                     ::Channel::Event::SetFrequency{static_cast<float>(playback_frequency)}});
            }
        }

        channel.volume =
            std::clamp(channel.volume, static_cast<int8_t>(0), static_cast<int8_t>(64));
        if (channel.volume != last_volume) {
            mixer_events.push_back(
                {static_cast<size_t>(channel_index),
                 ::Channel::Event::SetVolume{static_cast<float>(channel.volume) / 64.0f}});
        }
    }

    if (initial_tick) {
        tick_counter = speed;
        if (process_row == 0xFFFE ||
            ++process_row >= module->patterns[module->patternOrder[current_order]].row_count()) {
            current_order++;
            // TODO: End of order value 255 needs a name
            if (module->patternOrder[current_order] == 255) {
                current_order = 0;
            }
            process_row = break_row;
            break_row = 0;
        }
        current_row = process_row;
    }

    return mixer_events;
}
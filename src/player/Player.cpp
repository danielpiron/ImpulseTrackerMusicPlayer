#include "Player.h"
#include "Mixer.h"
#include "Module.h"

#include <algorithm>
#include <iostream>

static const int8_t sine_table[256] = {
    0,   2,   3,   5,   6,   8,   9,   11,  12,  14,  16,  17,  19,  20,  22,  23,  24,  26,  27,
    29,  30,  32,  33,  34,  36,  37,  38,  39,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
    51,  52,  53,  54,  55,  56,  56,  57,  58,  59,  59,  60,  60,  61,  61,  62,  62,  62,  63,
    63,  63,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  63,  63,  63,  62,  62,  62,
    61,  61,  60,  60,  59,  59,  58,  57,  56,  56,  55,  54,  53,  52,  51,  50,  49,  48,  47,
    46,  45,  44,  43,  42,  41,  39,  38,  37,  36,  34,  33,  32,  30,  29,  27,  26,  24,  23,
    22,  20,  19,  17,  16,  14,  12,  11,  9,   8,   6,   5,   3,   2,   0,   -2,  -3,  -5,  -6,
    -8,  -9,  -11, -12, -14, -16, -17, -19, -20, -22, -23, -24, -26, -27, -29, -30, -32, -33, -34,
    -36, -37, -38, -39, -41, -42, -43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55,
    -56, -56, -57, -58, -59, -59, -60, -60, -61, -61, -62, -62, -62, -63, -63, -63, -64, -64, -64,
    -64, -64, -64, -64, -64, -64, -64, -64, -63, -63, -63, -62, -62, -62, -61, -61, -60, -60, -59,
    -59, -58, -57, -56, -56, -55, -54, -53, -52, -51, -50, -49, -48, -47, -46, -45, -44, -43, -42,
    -41, -39, -38, -37, -36, -34, -33, -32, -30, -29, -27, -26, -24, -23, -22, -20, -19, -17, -16,
    -14, -12, -11, -9,  -8,  -6,  -5,  -3,  -2,
};

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
      _mixer(44100, 32)
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
        if (effect.data) {
            speed = effect.data;
            tick_counter = speed;
        }
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
        _mixer.set_samples_per_tick(static_cast<size_t>(2.5f * _mixer.sampling_rate() / tempo));
        break;
    default:
        break;
    }
}

int Player::calculate_period(const PatternEntry::Note& note, const int c5_speed)
{
    static const int note_periods[] = {1712, 1616, 1524, 1440, 1356, 1280,
                                       1208, 1140, 1076, 1016, 960,  907};
    return ((8363 * 32 * note_periods[note.index()]) >> note.octave()) / c5_speed;
}

int Player::sample_playback_rate(int sample_number) const
{
    return static_cast<int>(
        module->samples[static_cast<size_t>(sample_number - 1)].sample.playbackRate());
}

void Player::process_initial_tick(Player::Channel& channel, const PatternEntry& entry)
{
    bool candidate_note = false;
    if (!entry.note.is_empty()) {
        channel.last_note = entry.note;
        candidate_note = true;
    }
    if (entry.inst) {
        channel.last_inst = entry.inst;
        candidate_note = true;
    }

    if (candidate_note && channel.last_note.is_playable() && channel.last_inst) {
        if (entry.effect.comm != PatternEntry::Command::portamento_to_note) {
            channel.note_on = true;
            channel.period =
                calculate_period(channel.last_note, sample_playback_rate(channel.last_inst));
        }
        channel.volume = module->samples[channel.last_inst - 1].default_volume;
    }

    switch (entry.volume_effect.comm) {
    case PatternEntry::Command::set_volume:
        channel.volume = static_cast<int8_t>(entry.volume_effect.data);
        break;
    default:
        break;
    }

    channel.effects.arrpegio_offsets.fill(0);
    channel.effects.volume_slide_speed = 0;
    if (entry.effect.comm != PatternEntry::Command::vibrato &&
        entry.effect.comm != PatternEntry::Command::vibrato_and_volume_slide) {
        channel.effects.vibrato.speed = 0;
        channel.effects.vibrato.depth = 0;
        channel.period_offset = 0;
    }
    if (entry.effect.comm != PatternEntry::Command::portamento_to_and_volume_slide) {
        channel.effects.pitch_slide_speed = 0;
    }
    if (entry.effect.comm == PatternEntry::Command::volume_slide ||
        entry.effect.comm == PatternEntry::Command::portamento_to_and_volume_slide ||
        entry.effect.comm == PatternEntry::Command::vibrato_and_volume_slide) {
        auto data = entry.effect.data ? entry.effect.data : channel.effects_memory.volume_slide;
        channel.effects_memory.volume_slide = data;
        if (data.hi_nibble() == 0xF) {
            // Fine slide down
            channel.volume -= data.lo_nibble();
        } else if (data.lo_nibble() == 0xF) {
            // Fine slide up
            channel.volume += data.hi_nibble();
        } else if (data.lo_nibble() && data.hi_nibble() == 0) {
            // Slide down
            channel.effects.volume_slide_speed = -static_cast<int8_t>(data.lo_nibble());
        } else if (data.hi_nibble() && data.lo_nibble() == 0) {
            // Slide up
            channel.effects.volume_slide_speed = static_cast<int8_t>(data.hi_nibble());
        }
    } else if (entry.effect.comm == PatternEntry::Command::pitch_slide_down) {
        auto data = entry.effect.data ? entry.effect.data : channel.effects_memory.pitch_slide;
        channel.effects_memory.pitch_slide = data;
        if (data.hi_nibble() == 0xE) {
            channel.period += data.lo_nibble();
        } else if (data.hi_nibble() == 0xF) {
            channel.period += data.lo_nibble() * 4;
        } else {
            channel.effects.pitch_slide_speed = static_cast<int8_t>(data * 4);
            channel.effects.pitch_slide_target = 54784 + 1;
        }
    } else if (entry.effect.comm == PatternEntry::Command::pitch_slide_up) {
        auto data = entry.effect.data ? entry.effect.data : channel.effects_memory.pitch_slide;
        channel.effects_memory.pitch_slide = data;
        if (data.hi_nibble() == 0xE) {
            channel.period -= data.lo_nibble();
        } else if (data.hi_nibble() == 0xF) {
            channel.period -= data.lo_nibble() * 4;
        } else {
            channel.effects.pitch_slide_speed = -data * 4;
            channel.effects.pitch_slide_target = 56 - 1;
        }
    } else if (entry.effect.comm == PatternEntry::Command::portamento_to_note) {
        auto data = entry.effect.data ? entry.effect.data : channel.effects_memory.pitch_slide;
        channel.effects_memory.pitch_slide = data;

        channel.effects.pitch_slide_target =
            calculate_period(channel.last_note, sample_playback_rate(channel.last_inst));
        channel.effects.pitch_slide_speed = data * 4;

        if (channel.period > channel.effects.pitch_slide_target) {
            channel.effects.pitch_slide_speed = -channel.effects.pitch_slide_speed;
        }
    } else if (entry.effect.comm == PatternEntry::Command::vibrato) {
        auto data = entry.effect.data;
        if ((data & 0xF0) == 0) {
            data |= channel.effects_memory.vibrato & 0xF0;
        }
        if ((data & 0x0F) == 0) {
            data |= channel.effects_memory.vibrato & 0x0F;
        }
        channel.effects_memory.vibrato = data;

        channel.effects.vibrato.speed = (data >> 4) * 4;
        channel.effects.vibrato.depth = (data & 0x0F) * 4;
    } else if (entry.effect.comm == PatternEntry::Command::arpeggio) {
        int playback_rate = sample_playback_rate(channel.last_inst);
        auto first_period =
            calculate_period(channel.last_note + (entry.effect.data >> 4), playback_rate);
        auto second_period =
            calculate_period(channel.last_note + (entry.effect.data & 0x0f), playback_rate);

        channel.effects.arrpegio_offsets[0] = 0;
        channel.effects.arrpegio_offsets[1] = first_period - channel.period;
        channel.effects.arrpegio_offsets[2] = second_period - channel.period;
    } else if (entry.effect.comm == PatternEntry::Command::set_sample_offset) {
        channel.effects.sample_offset = entry.effect.data * 256;
    }
}

static void update_effects(Player::Channel& channel, int tick)
{
    channel.volume += channel.effects.volume_slide_speed;
    if (channel.effects.pitch_slide_speed) {
        channel.period += channel.effects.pitch_slide_speed;

        if ((channel.effects.pitch_slide_speed > 0 &&
             channel.period > channel.effects.pitch_slide_target) ||
            (channel.effects.pitch_slide_speed < 0 &&
             channel.period < channel.effects.pitch_slide_target)) {
            channel.period = channel.effects.pitch_slide_target;
            channel.effects.pitch_slide_speed = 0;
            channel.effects.pitch_slide_target = 0;
        }
    }
    if (channel.effects.vibrato.speed) {
        channel.effects.vibrato.index += channel.effects.vibrato.speed;
        channel.period_offset =
            (sine_table[channel.effects.vibrato.index] * channel.effects.vibrato.depth) >> 5;
    }
    if (channel.effects.arrpegio_offsets[0] == 0 &&
        (channel.effects.arrpegio_offsets[1] != 0 || channel.effects.arrpegio_offsets[2] != 0)) {
        channel.period_offset = channel.effects.arrpegio_offsets[tick % 3];
    }
}

const std::vector<Mixer::Event>& Player::process_tick()
{
    mixer_events.clear();

    bool initial_tick = --tick_counter == 0;
    if (initial_tick) {
        tick_counter = speed;
    }

    const auto& current_pattern = module->patterns[module->patternOrder[current_order]];
    for (size_t channel_index = 0; channel_index < current_pattern.channel_count();
         ++channel_index) {

        auto& channel = channels[static_cast<size_t>(channel_index)];

        auto last_volume = channel.volume;
        auto last_frequency = channel.frequency;

        channel.effects.sample_offset = 0;

        if (initial_tick) {
            const auto& entry = current_pattern.channel(channel_index).row(current_row);
            process_global_command(entry.effect);
            process_initial_tick(channel, entry);
        } else {
            update_effects(channel, speed - tick_counter);
        }

        if (channel.period + channel.period_offset > 0) {
            channel.frequency = 14317456 / (channel.period + channel.period_offset);
        }

        if (channel.note_on || channel.frequency != last_frequency) {
            if (channel.note_on) {
                mixer_events.push_back({static_cast<size_t>(channel_index),
                                        ::Channel::Event::SetNoteOn{
                                            static_cast<float>(channel.frequency),
                                            &(module->samples[channel.last_inst - 1].sample)}});
                channel.note_on = false;
            } else {
                mixer_events.push_back(
                    {static_cast<size_t>(channel_index),
                     ::Channel::Event::SetFrequency{static_cast<float>(channel.frequency)}});
            }
        }

        channel.volume =
            std::clamp(channel.volume, static_cast<int8_t>(0), static_cast<int8_t>(64));
        if (channel.volume != last_volume) {
            mixer_events.push_back(
                {static_cast<size_t>(channel_index),
                 ::Channel::Event::SetVolume{static_cast<float>(channel.volume) / 64.0f}});
        }

        if (channel.effects.sample_offset > 0) {
            mixer_events.push_back(
                {static_cast<size_t>(channel_index),
                 ::Channel::Event::SetSampleIndex{channel.effects.sample_offset}});
        }
    }

    if (initial_tick) {
        if (process_row == 0xFFFE ||
            ++process_row >= module->patterns[module->patternOrder[current_order]].row_count()) {
            while (module->patternOrder[++current_order] == 254)
                ;
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
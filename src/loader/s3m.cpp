#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <player/Module.h>
#include <player/PatternEntry.h>
#include <player/Sample.h>

template <typename T> T read(std::istream& is)
{
    T temp = 0;
    is.read(reinterpret_cast<char*>(&temp), sizeof(T));
    return temp;
}

struct InstrumentMetaData {
    uint16_t para_pointer;
    uint16_t length;
    uint16_t hi_length;
    uint16_t loop_begin;
    uint16_t hi_loop_Begin;
    uint16_t loop_end;
    uint16_t hi_loop_end;
    uint8_t default_vol;
    uint8_t padding4;
    uint8_t pack;
    uint8_t flags;
    uint16_t sampling_rate;
};

// Raise 8-bit value to higher 8-bits and
static uint16_t convert8to16(const uint8_t b)
{
    return static_cast<uint16_t>(b << 8 | (rand() & 0xFF));
}
static float convert(const uint16_t word)
{
    return static_cast<float>(word) / 65535.0f * 2.0f - 1.0f;
}

Module::Sample load_sample(std::ifstream& fs)
{
    InstrumentMetaData meta;

    fs.seekg(0x0E, std::ios::cur);
    fs.read(reinterpret_cast<char*>(&meta), sizeof(meta));

    std::vector<uint8_t> raw_data;
    raw_data.resize(meta.length);

    fs.seekg(meta.para_pointer * 16);
    fs.read(reinterpret_cast<char*>(&raw_data[0]), meta.length);

    std::vector<float> float_data;

    std::transform(raw_data.begin(), raw_data.end(), std::back_inserter(float_data),
                   [](const auto b) { return convert(convert8to16(b)); });

    bool is_looping = meta.flags & 1;
    auto loop_params = is_looping ? Sample::LoopParams{Sample::LoopParams::Type::forward_looping,
                                                       meta.loop_begin, meta.loop_end}
                                  : Sample::LoopParams{Sample::LoopParams::Type::non_looping};

    return Module::Sample{{float_data.begin(), float_data.end(), meta.sampling_rate, loop_params},
                          meta.default_vol};
}

PatternEntry::Command s3m_comm_to_effect(const uint8_t comm)
{
    // Offset the command with it's letter representation for easier reading
    char letter = static_cast<char>(comm - 1) + 'A';
    switch (letter) {
    case 'A':
        return PatternEntry::Command::set_speed;
    case 'B':
        return PatternEntry::Command::jump_to_order;
    case 'C':
        return PatternEntry::Command::break_to_row;
    case 'D':
        return PatternEntry::Command::volume_slide;
    case 'E':
        return PatternEntry::Command::pitch_slide_down;
    case 'F':
        return PatternEntry::Command::pitch_slide_up;
    case 'G':
        return PatternEntry::Command::portamento_to_note;
    case 'H':
        return PatternEntry::Command::vibrato;
    case 'K':
        return PatternEntry::Command::vibrato_and_volume_slide;
    case 'J':
        return PatternEntry::Command::arpeggio;
    case 'L':
        return PatternEntry::Command::portamento_to_and_volume_slide;
    case 'O':
        return PatternEntry::Command::set_sample_offset;
    case 'T':
        return PatternEntry::Command::set_tempo;
    default:
        return PatternEntry::Command::none;
    }
}

Pattern load_pattern(std::ifstream& fs)
{
    Pattern pattern(64);
    auto data_length = read<uint16_t>(fs);
    std::vector<uint8_t> buffer(data_length);

    fs.read(reinterpret_cast<char*>(&buffer[0]), data_length);

    auto data = buffer.cbegin();
    int row = 0;

    while (data != buffer.end() && row < 64) {
        auto control = *data++;
        if (control == 0) {
            row++;
            continue;
        }

        PatternEntry entry;
        int channel = control & 31;
        if (control & 32) {
            // byte 0 - Note; hi=oct, lo=note, 255=empty note,↲
            //          254=key off
            const uint8_t empty_note = 255;
            const uint8_t key_off = 254;
            uint8_t note = *data++;
            if (note == key_off) {
                entry._note = PatternEntry::Note{PatternEntry::Note::Type::note_off};
            } else if (note != empty_note) {
                uint8_t octave = (note >> 4) + 1;
                note &= 0x0F;
                entry._note = PatternEntry::Note{note, octave};
            }
            uint8_t inst = *data++;
            entry._inst = inst;
        }

        if (control & 64) {
            uint8_t vol = *data++;
            entry._volume_effect = {PatternEntry::Command::set_volume, vol};
        }

        if (control & 128) {
            uint8_t comm = *data++;
            uint8_t info = *data++;

            auto command = s3m_comm_to_effect(comm);
            if (command == PatternEntry::Command::break_to_row) {
                info = (info >> 4) * 10 + (info & 0x0F);
            }
            entry._effect = PatternEntry::Effect{command, info};
        }
        pattern.channel(static_cast<size_t>(channel)).row(static_cast<size_t>(row)) = entry;
    }
    return pattern;
}

std::shared_ptr<Module> load_s3m(std::ifstream& s3m)
{
    auto mod = std::make_shared<Module>();

    if (!s3m.is_open()) {
        std::cerr << "BAH!" << std::endl;
        return mod;
    }

    s3m.seekg(0x20);
    auto ord_num = read<uint16_t>(s3m);
    auto ins_num = read<uint16_t>(s3m);
    auto pat_num = read<uint16_t>(s3m);

    s3m.seekg(0x31);
    mod->initial_speed = read<uint8_t>(s3m);
    mod->initial_tempo = read<uint8_t>(s3m);

    mod->patternOrder.resize(ord_num);
    s3m.seekg(0x60);
    s3m.read(reinterpret_cast<char*>(&mod->patternOrder[0]), ord_num);

    std::vector<uint16_t> instrument_pointers;
    instrument_pointers.resize(ins_num);
    s3m.read(reinterpret_cast<char*>(&instrument_pointers[0]),
             sizeof(instrument_pointers[0]) * ins_num);

    std::vector<uint16_t> pattern_pointers;
    pattern_pointers.resize(pat_num);
    s3m.read(reinterpret_cast<char*>(&pattern_pointers[0]), sizeof(pattern_pointers[0]) * pat_num);

    // Load Samples
    for (const auto pointer : instrument_pointers) {
        s3m.seekg(pointer * 16);
        mod->samples.emplace_back(load_sample(s3m));
    }

    // Load Patterns
    for (const auto pointer : pattern_pointers) {
        s3m.seekg(pointer * 16);
        mod->patterns.emplace_back(load_pattern(s3m));
    }

    return mod;
}
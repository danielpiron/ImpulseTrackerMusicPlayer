
#include <player/Module.h>
#include <player/PatternEntry.h>

#include <algorithm>
#include <array>
#include <fstream>

template <typename T> static T read(std::istream& is)
{
    T temp = 0;
    is.read(reinterpret_cast<char*>(&temp), sizeof(T));
    return temp;
}

PatternEntry::Command it_comm_to_effect(const uint8_t comm)
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

static Pattern load_pattern(std::ifstream& fs)
{
    auto data_length = read<uint16_t>(fs);
    auto row_count = read<uint16_t>(fs);
    fs.seekg(4, std::ios::cur);

    std::vector<uint8_t> buffer(data_length);

    Pattern pattern(row_count);
    fs.read(reinterpret_cast<char*>(&buffer[0]), data_length);

    std::array<uint8_t, 64> last_mask_variables;
    std::array<PatternEntry, 64> last_entries;

    auto data = buffer.cbegin();
    int row = 0;
    while (data != buffer.end() && row < row_count) {
        auto channel_variable = *data++;
        if (channel_variable == 0) {
            row++;
            continue;
        }

        int channel = (channel_variable - 1) & 63;

        auto& last_entry = last_entries[static_cast<size_t>(channel)];
        auto& last_mask_variable = last_mask_variables[static_cast<size_t>(channel)];
        uint8_t mask_variable = last_mask_variable;

        if (channel_variable & 128) {
            mask_variable = *data++;
        }

        PatternEntry entry;
        if (mask_variable & 1) {
            const uint8_t note_cut = 255;
            const uint8_t note_off = 254;
            const uint8_t empty = 253;

            uint8_t note = *data++;
            if (note == empty) {
                entry.note = PatternEntry::Note{PatternEntry::Note::Type::empty};
            } else if (note == note_off) {
                entry.note = PatternEntry::Note{PatternEntry::Note::Type::note_off};
            } else if (note == note_cut) {
                entry.note = PatternEntry::Note{PatternEntry::Note::Type::note_cut};
            } else {
                entry.note = PatternEntry::Note{note % 12, note / 12};
            }
        }

        if (mask_variable & 2) {
            uint8_t inst = *data++;
            entry.inst = inst;
        }

        if (mask_variable & 4) {
            uint8_t vol = *data++;
            if (vol >= 0 && vol <= 64) {
                entry.volume_effect = {PatternEntry::Command::set_volume, vol};
            }
        }

        if (mask_variable & 8) {
            uint8_t comm = *data++;
            uint8_t info = *data++;

            auto command = it_comm_to_effect(comm);
            if (command == PatternEntry::Command::break_to_row) {
                info = (info >> 4) * 10 + (info & 0x0F);
            }
            entry.effect = PatternEntry::Effect{command, info};
        }

        if (mask_variable & 16) {
            entry.note = last_entry.note;
        }
        if (mask_variable & 32) {
            entry.inst = last_entry.inst;
        }
        if (mask_variable & 64) {
            entry.volume_effect = last_entry.volume_effect;
        }
        if (mask_variable & 128) {
            entry.effect = last_entry.effect;
        }

        pattern.channel(static_cast<size_t>(channel)).row(static_cast<size_t>(row)) = entry;
        last_mask_variable = mask_variable;
        last_entry = entry;
    }
    return pattern;
}

static Module::Sample load_sample(std::istream& is)
{
    struct SampleMetaData {
        uint32_t length;
        uint32_t loop_begin;
        uint32_t loop_end;
        uint32_t c5_speed;
        uint32_t sus_loop_begin;
        uint32_t sus_loop_end;
        uint32_t pointer;
    };

    auto sample_start = is.tellg();

    is.seekg(0x12 + sample_start);
    auto flags = read<uint8_t>(is);
    auto default_volume = read<uint8_t>(is);

    SampleMetaData meta;
    is.seekg(0x30 + sample_start);
    is.read(reinterpret_cast<char*>(&meta), sizeof(SampleMetaData));

    if (meta.length == 0) {
        return Sample({}, meta.c5_speed);
    }

    bool is_16bit = flags & 0x02;
    bool is_looping = flags & 0x10;

    // Locate sample data
    std::vector<float> samples;
    samples.reserve(meta.length);

    is.seekg(meta.pointer);
    if (is_16bit) {

    } else {
        std::vector<int8_t> raw_samples(meta.length);
        is.read(reinterpret_cast<char*>(&raw_samples[0]), meta.length);
        std::transform(raw_samples.begin(), raw_samples.end(), std::back_inserter(samples),
                       [](auto s) { return static_cast<float>(s) / 128.0f; });
    }

    Sample::LoopParams loop_params{Sample::LoopParams::Type::non_looping};
    if (is_looping) {
        loop_params = {Sample::LoopParams::Type::forward_looping, meta.loop_begin, meta.loop_end};
    }

    return Module::Sample{Sample(samples.begin(), samples.end(), meta.c5_speed, loop_params),
                          default_volume};
}

std::shared_ptr<Module> load_it(std::ifstream& it)
{
    auto mod = std::make_shared<Module>();

    if (!it.is_open()) {
        std::cerr << "BAH!" << std::endl;
        return mod;
    }

    it.seekg(0x20);
    auto ord_num = read<uint16_t>(it);
    auto ins_num = read<uint16_t>(it);
    auto smp_num = read<uint16_t>(it);
    auto pat_num = read<uint16_t>(it);

    it.seekg(0x32);
    // auto global_volume = read<uint8_t>(it);
    // auto mix_volume = read<uint8_t>(it);
    mod->initial_speed = read<uint8_t>(it);
    mod->initial_tempo = read<uint8_t>(it);

    mod->patternOrder.resize(ord_num);
    it.seekg(0xc0);
    it.read(reinterpret_cast<char*>(&(mod->patternOrder[0])), ord_num);

    std::vector<uint32_t> ins_pointers(ins_num);
    it.read(reinterpret_cast<char*>(&ins_pointers[0]), ins_num * sizeof(ins_pointers[0]));

    std::vector<uint32_t> smp_pointers(smp_num);
    it.read(reinterpret_cast<char*>(&smp_pointers[0]), smp_num * sizeof(smp_pointers[0]));

    std::vector<uint32_t> pat_pointers(pat_num);
    it.read(reinterpret_cast<char*>(&pat_pointers[0]), pat_num * sizeof(pat_pointers[0]));

    // We are skipping instruments for now
    for (const auto& pointer : smp_pointers) {
        it.seekg(pointer);
        mod->samples.emplace_back(load_sample(it));
    }

    for (const auto& pointer : pat_pointers) {
        if (pointer == 0) {
            // A pointer of zero indicates an empty 64 row pattern
            mod->patterns.emplace_back(64);
        } else {
            it.seekg(pointer);
            mod->patterns.emplace_back(load_pattern(it));
        }
    }

    return mod;
}

/*
int main()
{
    std::ifstream it("/Users/pironvila/Downloads/m4v-fasc.it", std::ios::binary);
    load_it(it);
    return 0;
}
*/
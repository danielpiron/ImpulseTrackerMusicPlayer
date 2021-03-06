#include "PatternEntry.h"

#include <cctype>
#include <cstring>
#include <iomanip>

static const char* note_symbols[] = {
    "C-", // 0
    "C#", // 1
    "D-", // 2
    "D#", // 3
    "E-", // 4
    "F-", // 5
    "F#", // 6
    "G-", // 7
    "G#", // 8
    "A-", // 9
    "A#", // 10
    "B-"  // 11
};

std::ostream& operator<<(std::ostream& os, const PatternEntry::Note& note)
{
    if (note.is_empty()) {
        os << "...";
    } else if (note.is_note_off()) {
        os << "---";
    } else if (note.is_note_cut()) {
        os << "^^^";
    } else {
        os << note_symbols[note.index()];
        os << note.octave();
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const PatternEntry& pe)
{
    os << pe.note << " ";

    if (pe.inst == 0) {
        os << "..";
    } else {
        auto width = os.width();
        auto fill = os.fill();
        os << std::setfill('0') << std::setw(2) << static_cast<int>(pe.inst);
        os << std::setfill(fill) << std::setw(static_cast<int>(width));
    }
    os << " ";

    // Start Handle volume
    if (pe.volume_effect.comm == PatternEntry::Command::set_volume) {
        os << static_cast<int>(pe.volume_effect.data);
    } else {
        os << "..";
    }
    os << " ";
    // End Handle
    switch (pe.effect.comm) {
    case PatternEntry::Command::none:
        os << '.';
        break;
    case PatternEntry::Command::set_speed:
        os << 'A';
        break;
    case PatternEntry::Command::jump_to_order:
        os << 'B';
        break;
    case PatternEntry::Command::break_to_row:
        os << 'C';
        break;
    case PatternEntry::Command::volume_slide:
        os << 'D';
        break;
    case PatternEntry::Command::pitch_slide_down:
        os << 'E';
        break;
    case PatternEntry::Command::pitch_slide_up:
        os << 'F';
        break;
    case PatternEntry::Command::portamento_to_note:
        os << 'G';
        break;
    case PatternEntry::Command::vibrato:
        os << 'H';
        break;
    case PatternEntry::Command::arpeggio:
        os << 'J';
        break;
    case PatternEntry::Command::vibrato_and_volume_slide:
        os << 'K';
        break;
    case PatternEntry::Command::portamento_to_and_volume_slide:
        os << 'L';
        break;
    case PatternEntry::Command::set_sample_offset:
        os << 'O';
        break;
    case PatternEntry::Command::set_tempo:
        os << 'T';
        break;
    default:
        os << '?';
    }

    auto width = os.width();
    auto fill = os.fill();
    os << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(pe.effect.data);
    os << std::setfill(fill) << std::setw(static_cast<int>(width)) << std::dec;

    return os;
}

int parse_hex_digit(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    return digit - 'A' + 10;
}

static PatternEntry::Note parse_pattern_entry_note(std::string::const_iterator& curr,
                                                   const std::string::const_iterator&)
{
    char buffer[4];
    for (size_t i = 0; i < 3; ++i) {
        buffer[i] = *curr++;
    }
    buffer[3] = '\0';

    if (std::strncmp(buffer, "^^^", 3) == 0)
        return PatternEntry::Note(PatternEntry::Note::Type::note_cut);

    PatternEntry::Note note;
    int octave = -1;
    if (std::isdigit(buffer[2])) {
        octave = buffer[2] - '0';
    }
    if (octave != -1) {
        for (int i = 0; i < 12; i++) {
            if (std::strncmp(&buffer[0], note_symbols[i], 2) == 0) {
                return PatternEntry::Note(i, octave);
            }
        }
    }
    return PatternEntry::Note();
}

static PatternEntry::Inst parse_pattern_entry_inst(std::string::const_iterator& curr,
                                                   const std::string::const_iterator&)
{
    char buffer[3];
    for (size_t i = 0; i < 2; ++i) {
        buffer[i] = *curr++;
    }
    buffer[2] = '\0';
    return static_cast<PatternEntry::Inst>(std::atoi(buffer));
}

static PatternEntry::Effect parse_pattern_entry_vol_effect(std::string::const_iterator& curr,
                                                           const std::string::const_iterator&)
{
    char buffer[3];
    for (size_t i = 0; i < 2; ++i) {
        buffer[i] = *curr++;
    }
    buffer[2] = '\0';

    PatternEntry::Command comm = PatternEntry::Command::none;
    int volume = 0;
    if (std::isdigit(buffer[0]) && std::isdigit(buffer[1])) {
        comm = PatternEntry::Command::set_volume;
        volume = std::atoi(buffer);
    }
    return PatternEntry::Effect(comm, volume);
}

static PatternEntry::Effect parse_pattern_entry_effect(std::string::const_iterator& curr,
                                                       const std::string::const_iterator&)
{
    char column[4];
    for (size_t i = 0; i < 3; ++i) {
        column[i] = *curr++;
    }
    column[3] = '\0';

    const char commandLetter = column[0];
    const int hi_data = parse_hex_digit(column[1]);
    const int lo_data = parse_hex_digit(column[2]);

    PatternEntry::Command comm = PatternEntry::Command::none;
    int data = hi_data << 4 | lo_data;
    switch (commandLetter) {
    case 'A':
        comm = PatternEntry::Command::set_speed;
        break;
    case 'B':
        comm = PatternEntry::Command::jump_to_order;
        break;
    case 'C':
        comm = PatternEntry::Command::break_to_row;
        break;
    case 'D':
        comm = PatternEntry::Command::volume_slide;
        break;
    case 'E':
        comm = PatternEntry::Command::pitch_slide_down;
        break;
    case 'F':
        comm = PatternEntry::Command::pitch_slide_up;
        break;
    case 'G':
        comm = PatternEntry::Command::portamento_to_note;
        break;
    case 'H':
        comm = PatternEntry::Command::vibrato;
        break;
    case 'J':
        comm = PatternEntry::Command::arpeggio;
        break;
    case 'K':
        comm = PatternEntry::Command::vibrato_and_volume_slide;
        break;
    case 'L':
        comm = PatternEntry::Command::portamento_to_and_volume_slide;
        break;
    case 'O':
        comm = PatternEntry::Command::set_sample_offset;
        break;
    case 'T':
        comm = PatternEntry::Command::set_tempo;
        break;
    }

    return {comm, static_cast<uint8_t>(data)};
}

void skip_whitespace(std::string::const_iterator& curr)
{
    while (std::isspace(*curr))
        curr++;
}

PatternEntry parse_pattern_entry(std::string::const_iterator& curr,
                                 const std::string::const_iterator& last)
{
    skip_whitespace(curr);
    auto note = parse_pattern_entry_note(curr, last);
    skip_whitespace(curr);
    auto inst = parse_pattern_entry_inst(curr, last);
    skip_whitespace(curr);
    auto vol_effect = parse_pattern_entry_vol_effect(curr, last);
    skip_whitespace(curr);
    auto effect = parse_pattern_entry_effect(curr, last);
    return PatternEntry(note, inst, vol_effect, effect);
}

PatternEntry parse_pattern_entry(const std::string& text)
{
    auto start = text.begin();
    return parse_pattern_entry(start, text.end());
}
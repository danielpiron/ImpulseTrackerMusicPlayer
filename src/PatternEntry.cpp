#include "PatternEntry.h"

#include <iomanip>

static const char* note_symbols[] = {
    "C-",  // 0
    "C#",  // 1
    "D-",  // 2
    "D#",  // 3
    "E-",  // 4
    "F-",  // 5
    "F#",  // 6
    "G-",  // 7
    "G#",  // 8
    "A-",  // 9
    "A#",  // 10
    "B-"   // 11
};

std::ostream& operator<<(std::ostream& os, const PatternEntry::Note& note) {
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

std::ostream& operator<<(std::ostream& os, const PatternEntry& pe) {
    os << pe._note << " ";

    if (pe._inst == 0) {
        os << "..";
    } else {
        auto width = os.width();
        auto fill = os.fill();
        os << std::setfill('0') << std::setw(2) << pe._inst;
        os << std::setfill(fill) << std::setw(static_cast<int>(width));
    }
    os << " ";

    // Start Handle volume
    os  << "..";
    os  << " ";
    // End Handle 
    if (pe._effect == PatternEntry::Command::none) {
        os << ".";
    } else {
        os << static_cast<char>('A' + pe._effect._comm - 1);
    }

    auto width = os.width();
    auto fill = os.fill();
    os << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(pe._effect._data);
    os << std::setfill(fill) << std::setw(static_cast<int>(width)) << std::dec;

    return os;
}

int parse_hex_digit(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    } 
    return digit - 'A' + 10;
}

PatternEntry::Effect parse_effects_column(const char* column) {
    const char commandLetter = column[0];
    const int hi_data = parse_hex_digit(column[1]);
    const int lo_data = parse_hex_digit(column[2]);

    PatternEntry::Command comm = PatternEntry::Command::none;
    int data = hi_data << 4 | lo_data;
    switch (commandLetter) {
        case 'A':
            comm = PatternEntry::Command::set_speed;
        break;

    }

    return {comm, static_cast<uint8_t>(data)};
}

PatternEntry parse_pattern_text(const std::string& text) {

    char buffer[14];

    strncpy(buffer, text.c_str(), 13);

    //        0123456789ABC
    //       "C-5 04 32 G10"
    //        |   |   |  +----- Effectj
    // Note --+   |   +-----+
    //            |         |
    //       Instrument   Volume
    //

    (void)note_symbols;
    // Insert null terminators between each segment
    buffer[3] = '\0';
    buffer[6] = '\0';
    buffer[9] = '\0';

    if (strncmp(&buffer[0], "^^^", 3) == 0) return PatternEntry::Note(PatternEntry::Note::Type::note_cut);

    PatternEntry::Note note;
    {
        int octave = -1;
        if (std::isdigit(buffer[2])) {
            octave = buffer[2] - '0';
        }
        if (octave != -1) {
            for (int i = 0; i < 12; i++) {
                if (strncmp(&buffer[0], note_symbols[i], 2) == 0) {
                    note = PatternEntry::Note(i, octave);
                    break;
                }
            }
        }
    }

    PatternEntry::Command comm = PatternEntry::Command::none;
    int volume = 0;
    if (std::isdigit(buffer[7]) && std::isdigit(buffer[8])) {
        comm  = PatternEntry::Command::set_volume;
        volume = atoi(&buffer[7]);
    }
    return PatternEntry(note, atoi(&buffer[4]), {comm, volume}, parse_effects_column(&buffer[10]));
}
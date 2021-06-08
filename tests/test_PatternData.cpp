#include <gtest/gtest.h>

/*
 C-4 01 .. .00 - will play note C octave 4, instrument 1
 D-4 .. .. .00 - will play note D, octave 4, instrument 1
 ... 02 .. .00 - Will play note D, octave 4, instrument 2
 E-4 .. .. .00 - will play note E, octave 4, instrument 2
 G-6 12 .. .00 - will play note G, octave 6, instrument 12
*/
/*

            ... .. .. G12     It is easier to   ... .. .. G12
            ... .. .. G12     use:              ... .. .. G00
            ... .. .. G12                       ... .. .. G00

           The following effects 'memorise' their previous values:
            (D/K/L), (E/F/G), (HU), I, J, N, O, S, T, W

           Note: Bracketed commands share the same 'memory' value. So

            ... .. .. E12   can be written as:  ... .. .. E12
            ... .. .. F12                       ... .. .. F00
            ... .. .. E12                       ... .. .. E00
            ... .. .. F12                       ... .. .. F00
            C-4 01 .. G12                       C-4 01 .. G00

            Commands H and U are linked even more closely.
            If you use H00 or U00, then the previous vibrato, no matter
            whether it was set with Hxx or Uxx will be used. So:

            ... .. .. H81    Is the same as:    ... .. .. H81
            ... .. .. U00                       ... .. .. H81
            ... .. .. U83                       ... .. .. U83
            ... .. .. U00                       ... .. .. U83
            ... .. .. H00                       ... .. .. U83
*/

struct PatternEntry {
    struct Note {
        Note(int note, int octave) : _value(octave * 12 + note) {}
        int _value;
        bool operator==(const Note& rhs) const {
            return _value == rhs._value;
        }
    };

    enum Command {
        none,
        set_volume
    };

    struct Effect {
        Effect(Command comm=Command::none, int data=0)
            : _comm(comm)
            , _data(data) {}
        Command _comm;
        int _data;

        bool operator==(const Effect& rhs) const {
            return _comm == rhs._comm && _data == rhs._data;
        }
    };

    PatternEntry() = default;
    PatternEntry(Note note, int inst=0, Effect vol=Effect())
    : _note(note)
    , _inst(inst)
    , _volume_effect(vol) {}

    bool operator==(const PatternEntry& rhs) const {
        return _note == rhs._note &&
               _inst == rhs._inst &&
               _volume_effect == rhs._volume_effect;
    }

    Note _note{-1, -1};
    int _inst = 0;
    Effect _volume_effect;
};

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

PatternEntry parse_pattern_text(const std::string& text) {

    char buffer[14];

    strncpy(buffer, text.c_str(), 13);

    //        0123456789ABC
    //       "C-5 04 32 G10"
    //        |   |   |  +----- Effect
    // Note --+   |   +-----+
    //            |         |
    //       Instrument   Volume
    //

    (void)note_symbols;
    // Insert null terminators between each segment
    buffer[3] = '\0';
    buffer[6] = '\0';
    buffer[9] = '\0';

    int octave = -1;
    if (std::isdigit(buffer[2])) {
        octave = buffer[2] - '0';
    }
    int note = -1;
    if (octave != -1) {
        for (int i = 0; i < 12; i++) {
            if (strncmp(&buffer[0], note_symbols[i], 2) == 0) {
                note = i;
                break;
            }
        }
    }

    PatternEntry::Command comm = PatternEntry::Command::none;
    int volume = 0;
    if (std::isdigit(buffer[7]) && std::isdigit(buffer[8])) {
        comm  = PatternEntry::Command::set_volume;
        volume = atoi(&buffer[7]);
    }

    return PatternEntry({note, octave}, atoi(&buffer[4]), {comm, volume});
}

TEST(ParsePatternsFromText, CanParseEmptyNotes) {
    PatternEntry expected;
    
    auto parsed_entry = parse_pattern_text("... .. .. .00");
    EXPECT_EQ(parsed_entry, expected);
}

TEST(ParsePatternsFromText, CanParseNote) {
    PatternEntry c_5({0, 5});
    PatternEntry fsharp3({6, 3});
    PatternEntry b_4({11, 4});
    PatternEntry empty;

    EXPECT_EQ(parse_pattern_text("C-5 .. .. .00"), c_5);
    EXPECT_EQ(parse_pattern_text("F#3 .. .. .00"), fsharp3);
    EXPECT_EQ(parse_pattern_text("B-4 .. .. .00"), b_4);
    // Invalid string gives an empty note
    EXPECT_EQ(parse_pattern_text("ABC .. .. .00"), empty);
    // First portion of note is acceptable, but octave is not
    EXPECT_EQ(parse_pattern_text("A-C .. .. .00"), empty);
}

TEST(ParsePatternsFromText, CanParseInstrument) {

    PatternEntry c_5_no_inst({0, 5});
    PatternEntry c_5_with_inst({0, 5}, 1);
    PatternEntry inst_alone({-1, -1}, 2);

    EXPECT_EQ(parse_pattern_text("C-5 .. .. .00"), c_5_no_inst);
    EXPECT_EQ(parse_pattern_text("C-5 01 .. .00"), c_5_with_inst);
    EXPECT_EQ(parse_pattern_text("... 02 .. .00"), inst_alone);
}

TEST(ParsePatternsFromText, CanParseSetVolume) {
    PatternEntry set_volume64({0, 5}, 0, {PatternEntry::Command::set_volume, 64});

    EXPECT_EQ(parse_pattern_text("C-5 .. 64 .00"), set_volume64);
}
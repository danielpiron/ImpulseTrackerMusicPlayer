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

    PatternEntry() = default;
    PatternEntry(Note note) : _note(note) {}
    bool operator==(const PatternEntry& rhs) const {
        return _note == rhs._note &&
               _inst == rhs._inst;
    }

    Note _note{-1, -1};
    int _inst = 0;
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
    // TODO: Check for invalid octave
    for (int i = 0; i < 12; i++) {
        if (strncmp(&buffer[0], note_symbols[i], 2) == 0) {
            note = i;
            break;
        }
    }
    PatternEntry pe({note, octave});
    pe._inst = atoi(&buffer[4]);
    return pe;
}

TEST(ParsePatternsFromText, CanParseEmptyNotes) {
    PatternEntry expected;
    
    auto parsed_entry = parse_pattern_text("... .. .. .00");
    EXPECT_EQ(parsed_entry, expected);
}

TEST(ParsePatternsFromText, CanParseInstrument) {
    PatternEntry expected;
    expected._inst = 4;
    
    auto parsed_entry = parse_pattern_text("... 04 .. .00");
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
}
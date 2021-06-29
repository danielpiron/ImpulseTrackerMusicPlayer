#include <gtest/gtest.h>

#include <PatternEntry.h>


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

TEST(ParsePatternsFromText, CanParseEmptyNotes) {
    PatternEntry expected;
    
    auto parsed_entry = parse_pattern_text("... .. .. .00");
    EXPECT_EQ(parsed_entry, expected);
}

TEST(ParsePatternsFromText, CanParseNote) {
    PatternEntry c_5(PatternEntry::Note{0, 5});
    PatternEntry fsharp3(PatternEntry::Note{6, 3});
    PatternEntry b_4(PatternEntry::Note{11, 4});
    PatternEntry empty;

    EXPECT_EQ(parse_pattern_text("C-5 .. .. .00"), c_5);
    EXPECT_EQ(parse_pattern_text("F#3 .. .. .00"), fsharp3);
    EXPECT_EQ(parse_pattern_text("B-4 .. .. .00"), b_4);
    // Invalid string gives an empty note
    EXPECT_EQ(parse_pattern_text("ABC .. .. .00"), empty);
    // First portion of note is acceptable, but octave is not
    EXPECT_EQ(parse_pattern_text("A-C .. .. .00"), empty);
}

TEST(ParsePatternsFromText, CanParseNoteCut) {
    EXPECT_EQ(parse_pattern_text("^^^ .. .. .00"), PatternEntry::Note(PatternEntry::Note::Type::note_cut));
}

TEST(ParsePatternsFromText, CanParseInstrument) {

    PatternEntry c_5_no_inst(PatternEntry::Note{0, 5});
    PatternEntry c_5_with_inst(PatternEntry::Note{0, 5}, 1);
    PatternEntry inst_alone(PatternEntry::Note(), 2);

    EXPECT_EQ(parse_pattern_text("C-5 .. .. .00"), c_5_no_inst);
    EXPECT_EQ(parse_pattern_text("C-5 01 .. .00"), c_5_with_inst);
    EXPECT_EQ(parse_pattern_text("... 02 .. .00"), inst_alone);
}

TEST(ParsePatternsFromText, CanParseSetVolume) {
    PatternEntry set_volume64(PatternEntry::Note{0, 5}, 0, {PatternEntry::Command::set_volume, 64});

    EXPECT_EQ(parse_pattern_text("C-5 .. 64 .00"), set_volume64);
}

TEST(ParsePatternsFromText, CanParseSetSpeed) {
    PatternEntry set_speed6(PatternEntry::Note{0, 4}, 0, PatternEntry::Effect(), {PatternEntry::Command::set_speed, 6});
    PatternEntry set_speed4(PatternEntry::Note(), 0, PatternEntry::Effect(), {PatternEntry::Command::set_speed, 4});

    EXPECT_EQ(parse_pattern_text("C-4 .. .. A06"), set_speed6);
    EXPECT_EQ(parse_pattern_text("... .. .. A04"), set_speed4);
}
#include <gtest/gtest.h>

#include <player/Pattern.h>

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

TEST(ParsePatternsFromText, CanParseEmptyNotes)
{
    PatternEntry expected;

    auto parsed_entry = parse_pattern_entry("... .. .. .00");
    EXPECT_EQ(parsed_entry, expected);
}

TEST(ParsePatternsFromText, CanSkipLeadingSpace)
{
    PatternEntry expected(PatternEntry::Note{0, 5});

    auto parsed_entry = parse_pattern_entry("   C-5 .. .. .00");
    EXPECT_EQ(parsed_entry, expected);
}

TEST(ParsePatternsFromText, CanParseNote)
{
    PatternEntry c_5(PatternEntry::Note{0, 5});
    PatternEntry fsharp3(PatternEntry::Note{6, 3});
    PatternEntry b_4(PatternEntry::Note{11, 4});
    PatternEntry empty;

    EXPECT_EQ(parse_pattern_entry("C-5 .. .. .00"), c_5);
    EXPECT_EQ(parse_pattern_entry("F#3 .. .. .00"), fsharp3);
    EXPECT_EQ(parse_pattern_entry("B-4 .. .. .00"), b_4);
    // Invalid string gives an empty note
    EXPECT_EQ(parse_pattern_entry("ABC .. .. .00"), empty);
    // First portion of note is acceptable, but octave is not
    EXPECT_EQ(parse_pattern_entry("A-C .. .. .00"), empty);
}

TEST(ParsePatternsFromText, CanParseNoteCut)
{
    EXPECT_EQ(parse_pattern_entry("^^^ .. .. .00"),
              PatternEntry::Note(PatternEntry::Note::Type::note_cut));
}

TEST(ParsePatternsFromText, CanParseInstrument)
{

    PatternEntry c_5_no_inst(PatternEntry::Note{0, 5});
    PatternEntry c_5_with_inst(PatternEntry::Note{0, 5}, 1);
    PatternEntry inst_alone(PatternEntry::Note(), 2);

    EXPECT_EQ(parse_pattern_entry("C-5 .. .. .00"), c_5_no_inst);
    EXPECT_EQ(parse_pattern_entry("C-5 01 .. .00"), c_5_with_inst);
    EXPECT_EQ(parse_pattern_entry("... 02 .. .00"), inst_alone);
}

TEST(ParsePatternsFromText, CanParseSetVolume)
{
    PatternEntry set_volume64(PatternEntry::Note(PatternEntry::Note::Type::empty), 0,
                              {PatternEntry::Command::set_volume, 64});

    EXPECT_EQ(parse_pattern_entry("... .. 64 .00"), set_volume64);
}

TEST(ParsePatternsFromText, CanParseSetSpeed)
{
    PatternEntry set_speed6(PatternEntry::Note{0, 4}, 0, PatternEntry::Effect(),
                            {PatternEntry::Command::set_speed, 6});
    PatternEntry set_speed4(PatternEntry::Note(), 0, PatternEntry::Effect(),
                            {PatternEntry::Command::set_speed, 4});

    EXPECT_EQ(parse_pattern_entry("C-4 .. .. A06"), set_speed6);
    EXPECT_EQ(parse_pattern_entry("... .. .. A04"), set_speed4);
}

TEST(ParsePatternsFromText, CanParseEmpty)
{
    Pattern expected(4);
    Pattern result(4);
    ASSERT_TRUE(parse_pattern("", expected));
    EXPECT_EQ(result, expected);
}

TEST(ParsePatternsFromText, CanParseSingleEntry)
{
    Pattern expected(4);
    Pattern result(4);

    expected.channel(0).row(0) = {
        PatternEntry::Note{0, 4}, 0, PatternEntry::Effect(), {PatternEntry::Command::set_speed, 6}};

    ASSERT_TRUE(parse_pattern("C-4 .. .. A06", result));
    EXPECT_EQ(result, expected);
}

TEST(ParsePatternsFromText, CanParseSingleChannel)
{
    using NoteName = Pattern::Entry::Note::Name;

    Pattern expected(8);
    Pattern result(8);

    expected.channel(0).row(0) = {PatternEntry::Note{NoteName::c_natural, 4}, 1};
    expected.channel(0).row(2) = {PatternEntry::Note{NoteName::e_natural, 4}, 1};
    expected.channel(0).row(4) = {PatternEntry::Note{NoteName::g_natural, 4}, 1};
    expected.channel(0).row(6) = {PatternEntry::Note{NoteName::c_natural, 5}, 1};

    auto text = R"(
    C-4 01 .. .00
    ... .. .. .00
    E-4 01 .. .00
    ... .. .. .00
    G-4 01 .. .00
    ... .. .. .00
    C-5 01 .. .00
    )";
    ASSERT_TRUE(parse_pattern(text, result));
    EXPECT_EQ(result, expected);
}

TEST(ParsePatternsFromText, CanParseMultipleChannels)
{
    using NoteName = Pattern::Entry::Note::Name;

    Pattern expected(8);
    Pattern result(8);

    expected.channel(0).row(0) = {PatternEntry::Note{NoteName::c_natural, 4}, 1};
    expected.channel(1).row(0) = {PatternEntry::Note{NoteName::e_natural, 4}, 1};
    expected.channel(1).row(1) = {PatternEntry::Note{NoteName::g_natural, 4}, 1};
    expected.channel(0).row(2) = {PatternEntry::Note{NoteName::c_natural, 5}, 1};

    auto text = R"(
    C-4 01 .. .00 E-4 01 .. .00
    ... .. .. .00 G-4 01 .. .00
    C-5 01 .. .00
    )";
    ASSERT_TRUE(parse_pattern(text, result));
    EXPECT_EQ(result, expected);
}

TEST(PatternEntryNotes, CanInitializeWithRawValue)
{
    using NoteName = Pattern::Entry::Note::Name;
    const PatternEntry::Note c5{NoteName::c_natural, 5};
    const PatternEntry::Note raw_c5{60};

    EXPECT_EQ(raw_c5, c5);
}

TEST(PatternEntryNotes, RawInitializationStaysWithinPlayableRange)
{
    using NoteName = Pattern::Entry::Note::Name;
    const PatternEntry::Note highest_note{NoteName::b_natural, 9};
    const PatternEntry::Note lowest_note{NoteName::c_natural, 0};

    EXPECT_EQ(PatternEntry::Note{-10}, lowest_note);
    EXPECT_EQ(PatternEntry::Note{254}, highest_note);
    // NOTE: 254 is a 'valid' note value, but we are restricting the 'int' constructor to playable
    // notes.
}

TEST(PatternEntryNotes, CanHaveOffsetsAdded)
{
    using NoteName = Pattern::Entry::Note::Name;
    const PatternEntry::Note c4{NoteName::c_natural, 4};
    const PatternEntry::Note e4{NoteName::e_natural, 4};
    const PatternEntry::Note g4{NoteName::g_natural, 4};
    const PatternEntry::Note c5{NoteName::c_natural, 5};
    const PatternEntry::Note b9{NoteName::b_natural, 9};

    EXPECT_EQ(c4 + 0, c4);
    EXPECT_EQ(c4 + 4, e4);
    EXPECT_EQ(c4 + 7, g4);
    EXPECT_EQ(c5 + -12, c4);
    EXPECT_EQ(b9 + 1, b9);
}

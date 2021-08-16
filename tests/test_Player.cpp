#include <gtest/gtest.h>

#include <player/Mixer.h>
#include <player/Module.h>
#include <player/Player.h>
#include <player/Sample.h>

#include <memory>

std::ostream& operator<<(std::ostream& os, const Mixer::Event& event)
{
    os << "Channel " << event.channel << " " << event.action;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Channel::Event::Action& action)
{
    struct ActionPrinter {
        void operator()(const Channel::Event::SetFrequency& sf)
        {
            os << "set frequency to " << sf.frequency;
        }
        void operator()(const Channel::Event::SetNoteOn& no)
        {
            os << "set note on (frequency: " << no.frequency << ", sample: " << no.sample;
        }
        void operator()(const Channel::Event::SetVolume& v) { os << "set volume to " << v.volume; }
        std::ostream& os;
    };

    std::visit(ActionPrinter{os}, action);

    return os;
}

class PlayerTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        mod = std::make_shared<Module>();
        mod->initial_speed = 1;
        mod->initial_tempo = 125;
        mod->patterns.resize(1, Pattern(8));
        mod->patternOrder = {0, 255};
        mod->samples.emplace_back(Sample{{0.5f, 1.0f, 0.5f, 1.0f}, 8363});
        mod->samples.emplace_back(Sample{{0.5f, 1.0f, 0.5f, 1.0f}, 8363 * 2});
    }
    void TearDown() override { mod = nullptr; }

    const std::vector<Mixer::Event> no_events{};
    std::shared_ptr<Module> mod;
};

class PlayerGlobalEffects : public PlayerTest {
};

class PlayerChannelEffects : public PlayerTest {
};

class PlayerNoteInterpretation : public PlayerTest {
};

class PlayerBehavior : public PlayerTest {
};

void advance_player(Player& player, size_t tick_count = 1)
{
    for (size_t i = 0; i < tick_count; i++) {
        player.process_tick();
    }
}

TEST_F(PlayerGlobalEffects, CanInheritInitialSpeedFromModule)
{
    Player player(mod);
    EXPECT_EQ(player.speed, mod->initial_speed);
    EXPECT_EQ(player.tempo, mod->initial_tempo);
}

TEST_F(PlayerGlobalEffects, CanHandleSetSpeedCommand)
{
    ASSERT_TRUE(parse_pattern(R"(... .. 00 A02
                                 ... .. 32 A03
                                 ... .. 00 ...)",
                              mod->patterns[0]));

    const std::vector<Mixer::Event> volume_to_zero{{0, Channel::Event::SetVolume{0}}};
    const std::vector<Mixer::Event> volume_to_half{{0, Channel::Event::SetVolume{0.5}}};

    Player player(mod);

    EXPECT_EQ(player.process_tick(), volume_to_zero);
    ASSERT_EQ(player.speed, 2);
    EXPECT_EQ(player.process_tick(), no_events);
    EXPECT_EQ(player.process_tick(), volume_to_half);
    ASSERT_EQ(player.speed, 3);
    EXPECT_EQ(player.process_tick(), no_events);
    EXPECT_EQ(player.process_tick(), no_events);
    EXPECT_EQ(player.process_tick(), volume_to_zero);
}

TEST_F(PlayerGlobalEffects, CanHandleJumpToOrderCommand)
{
    mod->patterns.resize(3, Pattern(8));
    mod->patternOrder = {0, 1, 2, 255};

    ASSERT_TRUE(parse_pattern(R"(... .. .. B02)", mod->patterns[0]));
    Player player(mod);
    player.process_tick();

    EXPECT_EQ(player.current_order, 2);
    EXPECT_EQ(player.current_row, 0);
}

TEST_F(PlayerGlobalEffects, CanHandleBreakToRowCommand)
{
    mod->patterns.resize(2, Pattern(8));
    mod->patternOrder = {0, 1, 255};

    ASSERT_TRUE(parse_pattern(R"(... .. .. C05 ... .. .. C03)", mod->patterns[0]));
    Player player(mod);
    player.process_tick();

    EXPECT_EQ(player.current_order, 1);
    EXPECT_EQ(player.current_row, 3);
}

TEST_F(PlayerGlobalEffects, CanHandleSetTempoCommand)
{
    ASSERT_TRUE(parse_pattern(R"(... .. .. T80)", mod->patterns[0]));
    Player player(mod);
    player.process_tick();

    EXPECT_EQ(player.tempo, 128);
}

TEST_F(PlayerChannelEffects, CanHandleFineVolumeSlide)
{
    const std::vector<Mixer::Event> volume_at_3_4ths{{0, Channel::Event::SetVolume{0.75f}}};

    ASSERT_TRUE(parse_pattern(R"(C-5 01 64 DF8
                                 ... .. .. D00
                                 C-5 01 32 D8F
                                 ... .. .. D00)",
                              mod->patterns[0]));

    mod->initial_speed = 2;
    Player player(mod);
    // START ROW 1
    { // Initial note of "Fine Volume Slide" will already experience decrease
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0, &mod->samples[0].sample}},
            {0, Channel::Event::SetVolume{1.0f - .125f}}};
        EXPECT_EQ(player.process_tick(), expected);
    }
    EXPECT_EQ(player.process_tick(), no_events);

    // START ROW 2
    // "Remember last setting"
    EXPECT_EQ(player.process_tick(), volume_at_3_4ths);
    EXPECT_EQ(player.process_tick(), no_events);

    // START ROW 3
    { // Initial note of "Fine Volume Slide" will already experience incerase
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0, &mod->samples[0].sample}},
            {0, Channel::Event::SetVolume{.5f + .125f}}};
        EXPECT_EQ(player.process_tick(), expected);
    }
    EXPECT_EQ(player.process_tick(), no_events);

    // START ROW 4
    // "Remember last setting"
    EXPECT_EQ(player.process_tick(), volume_at_3_4ths);
    EXPECT_EQ(player.process_tick(), no_events);
}

TEST_F(PlayerChannelEffects, CanHandleVolumeSlide)
{
    // Slide memory is already proven by fine slide, so we'll stick with
    // the inter row events.
    ASSERT_TRUE(parse_pattern(R"(C-5 01 64 D08
                                 ... .. .. .00
                                 C-5 01 32 D80)",
                              mod->patterns[0]));
    // Set speed to 3. We want two ticks of change.
    mod->initial_speed = 3;
    Player player(mod);

    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 64);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 56);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 48);

    // Intervening row should have no events for 3 ticks
    EXPECT_EQ(player.process_tick(), no_events);
    EXPECT_EQ(player.process_tick(), no_events);
    EXPECT_EQ(player.process_tick(), no_events);

    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 32);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 40);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 48);
}

TEST_F(PlayerChannelEffects, VolumeRangeIsClamped)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 66 D8F
                                 ... .. 62 D80
                                 C-5 01 02 D08
                                 ... .. .. DF8)",
                              mod->patterns[0]));

    mod->initial_speed = 2;
    Player player(mod);
    // START ROW 1
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 64);
    EXPECT_EQ(player.process_tick(),
              no_events); // No events as we have clamped to 64
    player.process_tick();

    // START ROW 2
    EXPECT_EQ(player.channels[0].volume, 62);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 64);
    player.process_tick();

    // START ROW 3
    EXPECT_EQ(player.channels[0].volume, 2);
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 0);

    // START ROW 4
    player.process_tick();
    EXPECT_EQ(player.channels[0].volume, 0);
}

TEST_F(PlayerNoteInterpretation, CanEmitVolumeChangeEvents)
{
    ASSERT_TRUE(parse_pattern(R"(... .. 00 .00
                                 ... .. 16 .00
                                 ... .. 32 .00
                                 ... .. 64 .00)",
                              mod->patterns[0]));

    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{{0, Channel::Event::SetVolume{0}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{{0, Channel::Event::SetVolume{0.25f}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{{0, Channel::Event::SetVolume{0.50f}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{{0, Channel::Event::SetVolume{1.0f}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerNoteInterpretation, CanEmitSetNoteOnEvents)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 .. .00
                                 E-5 01 .. .00
                                 G-5 01 .. .00
                                 C-6 01 .. .00)",
                              mod->patterns[0]));

    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{10558.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{12559.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{16726.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerNoteInterpretation, CanTriggerIncompleteNotes)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 .. .. .00
                                 ... 01 .. .00
                                 ... .. .. .00
                                 C-6 .. .. .00)",
                              mod->patterns[0]));
    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{16726.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerNoteInterpretation, SampleFrequencyAffectsSetFrequency)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 .. .00
                                 C-5 02 .. .00)",
                              mod->patterns[0]));

    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{16726.0f, &mod->samples[1].sample}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerNoteInterpretation, SamplesHaveDefaultVolume)
{
    ASSERT_TRUE(parse_pattern(R"(
                                C-5 03 .. .00
                                C-5 01 .. .00
                                )",
                              mod->patterns[0]));

    mod->samples.push_back(Module::Sample(Sample{{1.0f}, 8363}));
    mod->samples.back().default_volume = 32;

    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0f, &mod->samples[2].sample}},
            {0, Channel::Event::SetVolume{0.5f}}};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0f, &mod->samples[0].sample}},
            {0, Channel::Event::SetVolume{1.0f}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerNoteInterpretation, CanPlayMultipleChannels)
{
    ASSERT_TRUE(parse_pattern(R"(
                                C-4 02 .. .00 C-5 01 .. .00
                                )",
                              mod->patterns[0]));
    Player player(mod);
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0f, &mod->samples[1].sample}},
            {1, Channel::Event::SetNoteOn{8363.0f, &mod->samples[0].sample}}};
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerBehavior, PlayerLoopsAfterLastPattern)
{
    const auto row_count = 8;
    mod->patterns.resize(2, Pattern(row_count));
    mod->patternOrder = {0, 1, 255};

    Player player(mod);
    advance_player(player, row_count);
    // Advance passed the end of the first pattern
    EXPECT_EQ(player.current_row, 0);
    EXPECT_EQ(player.current_order, 1);

    advance_player(player, row_count);
    // Since there are only two pattenrs in the order,
    // repeating this process loops us back to the beginning
    EXPECT_EQ(player.current_row, 0);
    EXPECT_EQ(player.current_order, 0);
}

TEST_F(PlayerBehavior, PlayerRendersAudio)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 .. .00)", mod->patterns[0]));

    Player player(mod);

    float buffer;
    player.render_audio(&buffer, 1);

    const auto& mixer = player.mixer();
    EXPECT_EQ(mixer.channel(0).frequency(), 8363.0f);
}

TEST_F(PlayerBehavior, PlayerSpeedHasSignificance)
{
    ASSERT_TRUE(parse_pattern(R"(
        ... .. 32 .00
        C-5 01 .. .00
    )",
                              mod->patterns[0]));
    mod->initial_speed = 4;

    Player player(mod);

    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{{0, Channel::Event::SetVolume{0.50f}}};
        EXPECT_EQ(events, expected);
    }
    for (int i = 0; i < 3; ++i) {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        std::vector<Mixer::Event> expected{
            {0, Channel::Event::SetNoteOn{8363.0, &mod->samples[0].sample}},
            {0, Channel::Event::SetVolume{1.0f}}};
        EXPECT_EQ(events, expected);
    }
}
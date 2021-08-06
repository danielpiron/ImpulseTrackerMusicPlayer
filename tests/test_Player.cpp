#include <gtest/gtest.h>

#include <player/Module.h>
#include <player/Player.h>
#include <player/Sample.h>

#include <memory>

/*
Module mod;

v---- mod and player should be part of a test suite
mod.patterns.reserve(10);
mod.patterns.push_back(Pattern(64));

// are we going to use the same pattern with a module?
// We can expand or contract the amount of patterns.

So basically allow us to do whatever we want with modules interface-wise to
simplify things but the Player will hold a const shared_ptr

// Build 10 empty patterns
mod.patterns.resize(10);

parse_pattern("... .. .. A02", &mod.pattern[0])

parse_pattern("... .. .. A02")

// No reason we can't modify the Module after it's been assigned to the player.
// It's the player that's not allowed to mess with the Module as it holds a
// const pointer.

Player player(mod);

// produce some channel events, right? In this case, we changed speed internally
player.process_tick();

// Perhaps then we have two things (outputs). State changes and mixer events

EXPECT_EQ(player.speed, 2);

// Let's start off with processing some global effects as they are technically
// separate from the channel effects.
// Let's have alook at the IT format and see what some of the standard global
properties are.
// We have at least tempo and speed to worry about

*/

class PlayerTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        mod = std::make_shared<Module>();
        mod->initial_speed = 4;
        mod->initial_tempo = 125;
        mod->patterns.resize(1, Pattern(8));
        mod->patternOrder = {0, 255};
        mod->samples = {Sample{{0.5f, 1.0f, 0.5f, 1.0f}, 8363},
                        Sample{{0.5f, 1.0f, 0.5f, 1.0f}, 8363 * 2}};
    }
    void TearDown() override { mod = nullptr; }
    std::shared_ptr<Module> mod;
};

class PlayerGlobalEffects : public PlayerTest {
};

class PlayerChannelEvents : public PlayerTest {
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
    mod->patterns.resize(1, Pattern(8));
    mod->patternOrder = {0, 255};

    ASSERT_TRUE(parse_pattern(R"(... .. .. .00
                                 ... .. .. A06)",
                              mod->patterns[0]));
    Player player(mod);

    player.process_tick();
    EXPECT_EQ(player.speed, mod->initial_speed);
    player.process_tick();
    EXPECT_EQ(player.speed, 6);
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

    ASSERT_TRUE(parse_pattern(R"(... .. .. C03)", mod->patterns[0]));
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

TEST_F(PlayerChannelEvents, CanEmitVolumeChangeEvents)
{
    ASSERT_TRUE(parse_pattern(R"(... .. 64 .00
                                 ... .. 32 .00
                                 ... .. 16 .00
                                 ... .. 00 .00)",
                              mod->patterns[0]));
    Player player(mod);
    {
        const Player::Channel::Event expected{
            0, Player::Channel::Event::SetVolume{1.0f}};
        const auto& events = player.process_tick();
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0], expected);
    }
    {
        const Player::Channel::Event expected{
            0, Player::Channel::Event::SetVolume{0.5f}};
        const auto& events = player.process_tick();
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0], expected);
    }
    {
        const Player::Channel::Event expected{
            0, Player::Channel::Event::SetVolume{0.25f}};
        const auto& events = player.process_tick();
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0], expected);
    }
    {
        const Player::Channel::Event expected{
            0, Player::Channel::Event::SetVolume{0}};
        const auto& events = player.process_tick();
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0], expected);
    }
}

TEST_F(PlayerChannelEvents, CanEmitNotePlayingEvents)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 .. .00
                                 E-5 01 .. .00
                                 G-5 01 .. .00
                                 C-6 01 .. .00)",
                              mod->patterns[0]));

    Player player(mod);
    {
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{8363.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        const auto& events = player.process_tick();
        EXPECT_EQ(events, expected);
    }
    {
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{10558.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        const auto& events = player.process_tick();
        EXPECT_EQ(events, expected);
    }
    {
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{12559.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        const auto& events = player.process_tick();
        EXPECT_EQ(events, expected);
    }
    {
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{16726.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        const auto& events = player.process_tick();
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerChannelEvents, CanTriggerIncompleteNotes)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 .. .. .00
                                 ... 01 .. .00
                                 C-6 .. .. .00)",
                              mod->patterns[0]));
    Player player(mod);
    {
        const auto& events = player.process_tick();
        const std::vector<Player::Channel::Event> expected{};
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{8363.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{16726.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        EXPECT_EQ(events, expected);
    }
}

TEST_F(PlayerChannelEvents, SampleFrequencyAffectsSetFrequency)
{
    ASSERT_TRUE(parse_pattern(R"(C-5 01 .. .00
                                 C-5 02 .. .00)",
                              mod->patterns[0]));

    Player player(mod);
    {
        const auto& events = player.process_tick();
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{8363.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{1}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        EXPECT_EQ(events, expected);
    }
    {
        const auto& events = player.process_tick();
        const std::vector<Player::Channel::Event> expected{
            Player::Channel::Event{
                0, Player::Channel::Event::SetFrequency{16726.0f}},
            Player::Channel::Event{0, Player::Channel::Event::SetSample{2}},
            Player::Channel::Event{0, Player::Channel::Event::NoteOn{}},
        };
        EXPECT_EQ(events, expected);
    }
}
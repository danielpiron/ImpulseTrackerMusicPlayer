#include <gtest/gtest.h>

#include <player/Module.h>
#include <player/Player.h>

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

TEST(PlayerGlobalEffects, CanInheritInitialSpeedFromModule)
{
    auto mod = std::make_shared<Module>();

    mod->initial_speed = 4;
    mod->initial_tempo = 180;

    Player player(mod);

    EXPECT_EQ(player.speed, mod->initial_speed);
    EXPECT_EQ(player.tempo, mod->initial_tempo);
}
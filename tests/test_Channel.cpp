#include <gtest/gtest.h>

#include <player/Channel.h>

#include <vector>

TEST(Channel, NullSampleResultsInSilence)
{
    std::vector<float> buffer{1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> expected(buffer.size(), 0);

    Channel c;
    c.render(&buffer[0], buffer.size(), 1);

    ASSERT_EQ(buffer, expected);
}

TEST(Channel, CanHandleSingleSampleSample)
{
    std::vector<float> expected{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<float> buffer(expected.size());

    Sample sample({1.0f}, 1);
    Channel c;
    c.play(&sample);
    c.set_frequency(1.0);
    c.render(&buffer[0], 4, 1);

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanSpecifyFrequency)
{
    std::vector<float> expected{
        0.0f, 0.25f, 0.5f, 0.75f, 1.0f, // Render 1 - Frequency x1
        0.0,  0.5f,  1.0f,              // Render 2 - Frequency x2
        0.25, 0.375, 0.5,  0.625f       // Render 3 - Frequency x0.5 (with lerping)
    };
    std::vector<float> buffer(expected.size(), 0);
    Sample sample(expected.begin(), expected.begin() + 5, 1);

    Channel c;
    c.play(&sample);

    c.set_frequency(1.0);
    c.render(&buffer[0], 5, 1);
    // Double the frequency will skip values
    c.set_frequency(2.0);
    c.render(&buffer[5], 3, 1);
    // Half original frequency
    c.set_frequency(0.5);
    c.render(&buffer[8], 4, 1);

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanSpecifySampleRateOnRender)
{
    std::vector<float> expected{0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    std::vector<float> buffer(expected.size(), 0);
    Sample sample({0, 1.0f}, 1);

    Channel c;
    c.play(&sample);

    // Our sample is only two samples long
    c.set_frequency(1.0);
    // Rendering with a sample rate of 5 will expand it to fit 5 samples
    c.render(&buffer[0], buffer.size(), 4);

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanSetVolume)
{
    std::vector<float> expected{0, 0.5f, 1.0f, 0, 0.25f, 0.5f, 0, .125, .25f};
    std::vector<float> buffer(expected.size(), 0);

    Sample sample({0, 0.5f, 1.0f, 0, 0.5f, 1.0f, 0, 0.5f, 1.0f}, 1);

    Channel c;
    c.play(&sample);
    c.set_frequency(1.0);

    c.set_volume(1.0);
    c.render(&buffer[0], 3, 1);
    c.set_volume(0.5);
    c.render(&buffer[3], 3, 1);
    c.set_volume(0.25);
    c.render(&buffer[6], 3, 1);

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanRenderNonLoopingSample)
{
    // The sample plays then the channel goes silent
    std::vector<float> expected1{-1.0f, -0.5f, -0.25f, 0.25f};
    std::vector<float> expected2{0.5f, -1.0f, 0, 0};
    std::vector<float> expected3{0, 0, 0, 0};
    std::vector<float> buffer(4, 0);

    // The sample is 6 frames long
    Sample sample({-1.0f, -0.5f, -0.25f, 0.25f, 0.5f, -1.0f}, 1,
                  {Sample::LoopParams::Type::non_looping});
    Channel c;

    c.play(&sample);
    // And we are rendering 4 frames, (samples 0-3)
    c.render(&buffer[0], 4, 1);
    EXPECT_EQ(buffer, expected1);
    // And another 4 frames (samples 4-5), plus silence at the end
    c.render(&buffer[0], 4, 1);
    EXPECT_EQ(buffer, expected2);
    // Finally 4 more that should be 'silent'
    c.render(&buffer[0], 4, 1);
    EXPECT_EQ(buffer, expected3);

    // We expect two frames with the sample data, and two at zero
    EXPECT_FALSE(c.is_active());
}

TEST(Channel, CanBeStopped)
{
    std::vector<float> expected{-1.0f, 1.0f, 0, 0};
    std::vector<float> buffer(expected.size(), 0);

    Sample sample({-1.0f, 1.0f}, 1);
    Channel c;

    c.play(&sample);
    c.render(&buffer[0], 2, 1);
    c.stop(); // Simulates a note cut
    c.render(&buffer[2], 2, 1);

    EXPECT_EQ(buffer, expected);
}

TEST(ChannelEventsInterpretation, CanSetFrequency)
{
    Channel channel;

    Channel::Event::Action action{Channel::Event::SetFrequency{8363.0f}};
    channel.process_event(action);

    EXPECT_EQ(channel.frequency(), 8363.0f);
}

TEST(ChannelEventsInterpretation, CanSetVolume)
{
    Channel channel;

    Channel::Event::Action action{Channel::Event::SetVolume{0.5f}};
    channel.process_event(action);

    EXPECT_EQ(channel.volume(), 0.5f);
}

TEST(ChannelEventsInterpretation, CanSetNoteOn)
{
    Channel channel;
    Sample sample({1.0f}, 1);

    Channel::Event::Action action{Channel::Event::SetNoteOn{8363.0f, &sample}};
    channel.process_event(action);

    EXPECT_EQ(channel.frequency(), 8363.0f);
    EXPECT_EQ(channel.sample(), &sample);
    EXPECT_TRUE(channel.is_active());
}
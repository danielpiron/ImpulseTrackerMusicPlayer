#include <gtest/gtest.h>

#include <player/Mixer.h>

#include <algorithm>
#include <vector>

TEST(Mixer, CanAcceptOnTickHandler)
{
    struct TestHandler : public Mixer::TickHandler {
        void onAttachment(Mixer& audio) override { audio.set_samples_per_tick(3); }
        void onTick(Mixer& audio) override
        {
            (void)audio;
            ticks_processed++;
        }
        size_t ticks_processed = 0;
    };

    std::vector<float> buffer(8);

    TestHandler t;
    Mixer ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(t.ticks_processed, 3UL);
}

TEST(Mixer, CanSetTicksPerSecond)
{
    struct TestHandler : public Mixer::TickHandler {
        void onAttachment(Mixer& audio) override { audio.set_samples_per_tick(3); }
        void onTick(Mixer& audio) override
        {
            (void)audio;
            ticks_processed++;
        }
        size_t ticks_processed = 0;
    };

    TestHandler t;
    Mixer ag;
}

TEST(Mixer, CanRenderSingleChannel)
{
    // Demonstrate channel rendering with tick handler
    struct VolumeTweaker : public Mixer::TickHandler {

        VolumeTweaker() : one({1.0f}, 1), volume(1.0f) {}

        void onAttachment(Mixer& audio) override
        {
            audio.channel(0).set_frequency(1.0f);
            audio.channel(0).play(&one);
            audio.set_samples_per_tick(2);
        }
        void onTick(Mixer& audio) override
        {
            audio.channel(0).set_volume(volume);
            volume /= 2;
        }

        Sample one;
        float volume;
    };

    std::vector<float> expected{1.0f, 1.0f, 0.5f, 0.5, 0.25f, 0.25f, 0.125f, 0.125f};
    std::vector<float> buffer(expected.size());

    VolumeTweaker t;
    Mixer ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(buffer, expected);
}

TEST(Mixer, CanMixChannels)
{
    std::vector<float> expected{1.0f, 0.5f, 1.0f, 0.5};
    std::vector<float> buffer(expected.size());

    // Sampling rate of 1hz and 2 channels
    Mixer mixer(1, 2);

    Sample s1({1.0f, 0}, 1);
    Sample s2({0, 0.5f}, 1);
    mixer.channel(0).play(&s1);
    mixer.channel(1).play(&s2);

    mixer.render(&buffer[0], 4);
    EXPECT_EQ(buffer, expected);
}

TEST(Mixer, CanProcessMixerEvent)
{
    // Sampling rate of 1hz sampling rate and 2 channels
    Mixer mixer(1, 2);
    Sample sample({1.0f}, 1);

    Mixer::Event event1{0, Channel::Event::SetNoteOn{8363.0f, &sample}};
    Mixer::Event event2{1, Channel::Event::SetVolume{0.5f}};

    mixer.process_event(event1);
    mixer.process_event(event2);

    EXPECT_EQ(mixer.channel(0).frequency(), 8363.0f);
    EXPECT_EQ(mixer.channel(0).sample(), &sample);
}
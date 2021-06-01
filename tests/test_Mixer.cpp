#include <gtest/gtest.h>

#include <Mixer.h>

#include <algorithm>
#include <vector>

TEST(Mixer, CanAcceptOnTickHandler) {

    struct TestHandler : public Mixer::TickHandler {
        void onAttachment(Mixer& audio) override {
            audio.set_samples_per_tick(3);
        }
        void onTick(Mixer& audio) override {
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

TEST(Mixer, CanRenderSingleChannel) {
    // Demonstrate channel rendering with tick handler
    struct VolumeTweaker : public Mixer::TickHandler {
        void onAttachment(Mixer& audio) override {
            audio.channels[0].set_frequency(1.0f);
            audio.channels[0].play(&one);
            audio.set_samples_per_tick(2);
        }
        void onTick(Mixer& audio) override {
            audio.channels[0].set_volume(volume);
            volume /= 2;
        }

        Sample one = {1.0f};
        float volume = 1;
    };

    std::vector<float> expected{1.0f, 1.0f, 0.5f, 0.5, 0.25f, 0.25f, 0.125f, 0.125f};
    std::vector<float> buffer(expected.size());

    VolumeTweaker t;
    Mixer ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(buffer, expected);
}

TEST(Mixer, CanMixChannels) {
    std::vector<float> expected{1.0f, 0.5f, 1.0f, 0.5};
    std::vector<float> buffer(expected.size());

    // Sampling rate of 1hz and 2 channels
    Mixer ag(1, 2);

    Sample s1{1.0f, 0};
    Sample s2{0, 0.5f};
    ag.channels[0].play(&s1);
    ag.channels[1].play(&s2);

    ag.render(&buffer[0], 4);
    EXPECT_EQ(buffer, expected);
}

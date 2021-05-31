#include <gtest/gtest.h>

#include <algorithm>
#include <list>
#include <vector>

#include <Channel.h>

struct AudioGen {

    struct TickHandler {
        virtual void onAttachment(AudioGen& audio) = 0;
        virtual void onTick(AudioGen& audio) = 0;
        virtual ~TickHandler() = default;
    };

    AudioGen() : channels(1) {}

    void attach_handler(TickHandler* handler) {
        handlers.push_back(handler);
        handler->onAttachment(*this);
    }

    void render(float* outputBuffer, size_t samplesToFill) {

        (void)outputBuffer;

        while (samplesToFill) {
            if (samples_until_next_tick == 0) {
                for (auto handler : handlers) {
                    handler->onTick(*this);
                }
                samples_until_next_tick = samples_per_tick;
            }

            auto samples_to_render = std::min(samples_until_next_tick, samplesToFill);

            samplesToFill -= samples_to_render;
            samples_until_next_tick -= samples_to_render;
            for (auto& channel : channels) {
                channel.render(outputBuffer, samples_to_render, 1);
                outputBuffer += samples_to_render;
            }
        }
    }

    void set_samples_per_tick(size_t spt) {
        samples_per_tick = spt;
    }

    size_t samples_until_next_tick = 0;
    size_t samples_per_tick = 1;
    std::list<TickHandler*> handlers;
    std::vector<Channel> channels;
};

TEST(AudioGen, CanAcceptOnTickHandler) {

    struct TestHandler : public AudioGen::TickHandler {
        void onAttachment(AudioGen& audio) override {
            audio.set_samples_per_tick(3);
        }
        void onTick(AudioGen& audio) override {
            (void)audio;
            ticks_processed++;
        }
        size_t ticks_processed = 0;
    };

    std::vector<float> buffer(8);

    TestHandler t;
    AudioGen ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(t.ticks_processed, 3UL);
}

TEST(AudioGen, CanRenderSingleChannel) {
    // Demonstrate channel rendering with tick handler
    struct VolumeTweaker : public AudioGen::TickHandler {
        void onAttachment(AudioGen& audio) override {
            audio.channels[0].set_frequency(1.0f);
            audio.channels[0].set_sample(&one);
            audio.set_samples_per_tick(2);
        }
        void onTick(AudioGen& audio) override {
            audio.channels[0].set_volume(volume);
            volume /= 2;
        }

        Sample one{1.0f};
        float volume = 1;
    };

    std::vector<float> expected{1.0f, 1.0f, 0.5f, 0.5, 0.25f, 0.25f, 0.125f, 0.125f};
    std::vector<float> buffer(expected.size());

    VolumeTweaker t;
    AudioGen ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(buffer, expected);
}
#include <gtest/gtest.h>

#include <algorithm>
#include <list>
#include <vector>

struct AudioGen {

    struct TickHandler {
        virtual void onAttachment(AudioGen& audio) = 0;
        virtual void onTick(AudioGen& audio) = 0;
        virtual ~TickHandler() = default;
    };

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
        }
    }

    void set_samples_per_tick(size_t spt) {
        samples_per_tick = spt;
    }

    size_t samples_until_next_tick = 0;
    size_t samples_per_tick = 1;
    std::list<TickHandler*> handlers;
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

    std::vector<float> buffer(1);

    TestHandler t;
    AudioGen ag;

    ag.attach_handler(&t);
    ag.render(&buffer[0], 8);

    EXPECT_EQ(t.ticks_processed, 3UL);
}

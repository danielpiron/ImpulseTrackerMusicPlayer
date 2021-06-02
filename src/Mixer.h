#ifndef _MIXER_H_
#define _MIXER_H_

#include <list>
#include <vector>

#include <Channel.h>

struct Mixer {

    struct TickHandler {
        virtual void onAttachment(Mixer& audio) = 0;
        virtual void onTick(Mixer& audio) = 0;
        virtual ~TickHandler() = default;
    };

    Mixer(const unsigned int sample_rate_ = 1, const size_t channel_count = 1)
    : sample_rate(sample_rate_)
    , auxilliary_buffer(1024) 
    , channels(channel_count) {}

    void attach_handler(TickHandler* handler) {
        handlers.push_back(handler);
        handler->onAttachment(*this);
    }

    void render(float* outputBuffer, size_t samplesToFill) {

        memset(outputBuffer, 0, samplesToFill * sizeof(float));

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
                channel.render(&auxilliary_buffer[0], samples_to_render, sample_rate);
                for (size_t i = 0; i < samples_to_render; ++i) {
                    outputBuffer[i] += auxilliary_buffer[i];
                }
            }
            outputBuffer += samples_to_render;
        }
    }

    void set_samples_per_tick(size_t spt) {
        samples_per_tick = spt;
    }

    size_t samples_until_next_tick = 0;
    size_t samples_per_tick = 1;
    unsigned int sample_rate = 1;
    std::vector<float> auxilliary_buffer;

    std::list<TickHandler*> handlers;
    std::vector<Channel> channels;
};

#endif
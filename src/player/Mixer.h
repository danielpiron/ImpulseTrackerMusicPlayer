#ifndef _MIXER_H_
#define _MIXER_H_

#include <list>
#include <vector>

#include "Channel.h"

class Mixer {

public:
    struct TickHandler {
        virtual void onAttachment(Mixer& audio) = 0;
        virtual void onTick(Mixer& audio) = 0;
        virtual ~TickHandler() = default;
    };

    Mixer(const unsigned int sample_rate_ = 1, const size_t channel_count = 1)
    : _sample_rate(sample_rate_)
    , _auxilliary_buffer(1024) 
    , _channels(channel_count) {}

    void attach_handler(TickHandler* handler) {
        _handlers.push_back(handler);
        handler->onAttachment(*this);
    }

    void render(float* outputBuffer, size_t samplesToFill) {

        memset(outputBuffer, 0, samplesToFill * sizeof(float));

        while (samplesToFill) {
            if (_samples_until_next_tick == 0) {
                for (auto handler : _handlers) {
                    handler->onTick(*this);
                }
                _samples_until_next_tick = _samples_per_tick;
            }

            auto samples_to_render = std::min(_samples_until_next_tick, samplesToFill);

            samplesToFill -= samples_to_render;
            _samples_until_next_tick -= samples_to_render;
            for (auto& channel : _channels) {
                channel.render(&_auxilliary_buffer[0], samples_to_render, _sample_rate);
                for (size_t i = 0; i < samples_to_render; ++i) {
                    outputBuffer[i] += _auxilliary_buffer[i];
                }
            }
            outputBuffer += samples_to_render;
        }
    }

    Channel& channel(size_t c) {
        return _channels[c];
    }

    void set_samples_per_tick(size_t spt) {
        _samples_per_tick = spt;
    }

private:
    size_t _samples_until_next_tick = 0;
    size_t _samples_per_tick = 1;
    unsigned int _sample_rate = 1;
    std::vector<float> _auxilliary_buffer;

    std::list<TickHandler*> _handlers;
    std::vector<Channel> _channels;
};

#endif
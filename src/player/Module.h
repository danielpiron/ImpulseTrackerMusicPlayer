#ifndef _PLAYER_MODULE_H_
#define _PLAYER_MODULE_H_

#include <player/Pattern.h>
#include <player/Sample.h>

#include <cinttypes>
#include <vector>

struct Module {
    using Pattern = ::Pattern;

    struct Sample {
        Sample(const ::Sample&& mixer_sample) : sample(std::move(mixer_sample))
        {
        }
        ::Sample sample;
        int8_t default_volume = 64;
    };

    std::vector<Sample> samples;
    std::vector<Pattern> patterns;
    std::vector<uint8_t> patternOrder;
    int initial_speed;
    int initial_tempo;
};

#endif
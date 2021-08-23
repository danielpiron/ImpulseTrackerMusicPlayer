#ifndef _PLAYER_MODULE_H_
#define _PLAYER_MODULE_H_

#include <player/Pattern.h>
#include <player/Sample.h>

#include <cinttypes>
#include <vector>

struct Module {
    using Pattern = Pattern;

    struct Sample {
        Sample(const ::Sample&& mixer_sample, int default_volume = 64)
            : sample(std::move(mixer_sample)), default_volume(static_cast<int8_t>(default_volume))
        {
        }
        ::Sample sample;
        int8_t default_volume;
    };

    std::vector<Sample> samples;
    std::vector<Pattern> patterns;
    std::vector<uint8_t> patternOrder;
    int initial_speed;
    int initial_tempo;
};

#endif
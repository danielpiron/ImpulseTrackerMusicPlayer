#ifndef _PLAYER_MODULE_H_
#define _PLAYER_MODULE_H_

#include <player/Pattern.h>
#include <player/Sample.h>

#include <vector>

struct Module {
    using Pattern = Pattern;

    std::vector<Sample> samples;
    std::vector<Pattern> patterns;
    std::vector<uint8_t> patternOrder;
    int initial_speed;
    int initial_tempo;
};

#endif
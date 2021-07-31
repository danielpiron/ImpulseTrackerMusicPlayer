#ifndef _PLAYER_MODULE_H_
#define _PLAYER_MODULE_H_

#include <player/Pattern.h>

struct Module {
    using Pattern = Pattern;

    int initial_speed;
    int initial_tempo;
};

#endif
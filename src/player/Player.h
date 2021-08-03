#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <memory>
#include <player/PatternEntry.h>

#include <vector>

struct Module;
struct Player {

    struct ChannelEvent {
        enum class Type : uint8_t { volume };
        int channel;
        Type type;
        float value;

        bool operator==(const ChannelEvent& rhs) const
        {
            return channel == rhs.channel && type == rhs.type &&
                   value == rhs.value;
        }
    };

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();

    void process_global_command(const PatternEntry::Effect& effect);
    const std::vector<ChannelEvent>& process_tick();

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    size_t current_row;
    size_t current_order;
};

#endif
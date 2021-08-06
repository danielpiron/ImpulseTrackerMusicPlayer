#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <memory>
#include <player/PatternEntry.h>

#include <variant>
#include <vector>

struct Module;
struct Player {

    struct Channel {

        struct Event {
            struct NoteOn {
                bool operator==(const NoteOn&) const { return true; }
            };

            struct SetFrequency {
                float frequency;
                bool operator==(const SetFrequency& rhs) const
                {
                    return frequency == rhs.frequency;
                }
            };

            struct SetSample {
                int sampleIndex;
                bool operator==(const SetSample& rhs) const
                {
                    return sampleIndex == rhs.sampleIndex;
                }
            };

            struct SetVolume {
                float volume;
                bool operator==(const SetVolume& rhs) const
                {
                    return volume == rhs.volume;
                }
            };

            bool operator==(const Event& rhs) const
            {
                return channel == rhs.channel && action == rhs.action;
            }

            using Action =
                std::variant<NoteOn, SetFrequency, SetSample, SetVolume>;

          public:
            int channel;
            Action action;
        };

      public:
        PatternEntry::Note last_note;
        PatternEntry::Inst last_inst;
    };

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();

    void process_global_command(const PatternEntry::Effect& effect);
    const std::vector<Channel::Event>& process_tick();

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    size_t current_row;
    size_t current_order;
    std::vector<Channel> channels;
};

extern std::ostream& operator<<(std::ostream& os,
                                const Player::Channel::Event& event);
extern std::ostream& operator<<(std::ostream& os,
                                const Player::Channel::Event::Action& action);

#endif
#ifndef _PLAYER_PLAYER_H_
#define _PLAYER_PLAYER_H_

#include <player/Mixer.h>
#include <player/PatternEntry.h>

#include <memory>
#include <variant>
#include <vector>

struct Module;
struct Player : public Mixer::TickHandler {

  public:
    void onAttachment(Mixer& audio) override;
    void onTick(Mixer& audio) override;

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
        PatternEntry::Inst last_inst = 0;
    };

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();

    void render_audio(float*, int);
    void process_global_command(const PatternEntry::Effect& effect);

    const std::vector<Channel::Event>& process_tick();

    const Mixer& mixer() const { return _mixer; }

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    size_t current_row;
    size_t current_order;
    std::vector<Channel> channels;

  private:
    Mixer _mixer;
};

extern std::ostream& operator<<(std::ostream& os,
                                const Player::Channel::Event& event);
extern std::ostream& operator<<(std::ostream& os,
                                const Player::Channel::Event::Action& action);

#endif
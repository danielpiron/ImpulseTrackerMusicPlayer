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

        struct Effects {
            struct Vibrato {
                uint8_t index = 0;
                uint8_t speed = 0;
                uint8_t depth = 0;
            };

            int8_t volume_slide_speed = 0;
            int pitch_slide_speed = 0;
            int pitch_slide_target = 0;
            Vibrato vibrato;
        };

        struct EffectsMemory {
            uint8_t volume_slide = 0;
            uint8_t pitch_slide = 0;
            uint8_t vibrato = 0;
        };

      public:
        PatternEntry::Note last_note;
        PatternEntry::Inst last_inst = 0;

        Effects effects;
        EffectsMemory effects_memory;

      public:
        bool note_on = false;
        int period;
        int period_offset = 0;
        int8_t volume = 64;
    };

    Player(const std::shared_ptr<Module>& mod);

    void render_audio(float*, int);

    static int calculate_period(const PatternEntry::Note& note, const int c5_speed);
    void process_global_command(const PatternEntry::Effect& effect);
    void process_initial_tick(Player::Channel& channel, const PatternEntry& entry);

    const std::vector<Mixer::Event>& process_tick();

    const Mixer& mixer() const { return _mixer; }

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    int tick_counter;
    size_t break_row;
    size_t current_row;
    size_t current_order;
    size_t process_row;
    std::vector<Channel> channels;
    std::vector<Mixer::Event> mixer_events;

  private:
    Mixer _mixer;
};

#endif
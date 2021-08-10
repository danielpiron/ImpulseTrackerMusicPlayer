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
      public:
        PatternEntry::Note last_note;
        PatternEntry::Inst last_inst = 0;
    };

    Player(const std::shared_ptr<Module>& mod);

    const std::vector<PatternEntry>& next_row();

    void render_audio(float*, int);
    void process_global_command(const PatternEntry::Effect& effect);

    const std::vector<Mixer::Event>& process_tick();

    const Mixer& mixer() const { return _mixer; }

    std::shared_ptr<const Module> module;
    int speed;
    int tempo;
    int tick_counter;
    size_t current_row;
    size_t current_order;
    std::vector<Channel> channels;
    std::vector<Mixer::Event> mixer_events;

  private:
    Mixer _mixer;
};

#endif
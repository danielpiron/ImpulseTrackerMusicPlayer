#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "Sample.h"

#include <cstring>
#include <ostream>
#include <variant>

class Channel {
  public:
    struct Event {
        struct SetFrequency {
            float frequency;
            bool operator==(const SetFrequency& rhs) const
            {
                return frequency == rhs.frequency;
            }
        };
        struct SetNoteOn {
            float frequency;
            const Sample* sample;
            bool operator==(const SetNoteOn& rhs) const
            {
                return sample == rhs.sample && frequency == rhs.frequency;
            }
        };
        struct SetVolume {
            float volume;
            bool operator==(const SetVolume& rhs) const
            {
                return volume == rhs.volume;
            }
        };
        using Action = std::variant<SetFrequency, SetNoteOn, SetVolume>;
    };

  public:
    void process_event(const Event::Action& action)
    {
        struct ActionInterpreter {
            void operator()(const Event::SetFrequency& set_freq)
            {
                c.set_frequency(set_freq.frequency);
            }
            void operator()(const Event::SetNoteOn& note_on)
            {
                c.set_frequency(note_on.frequency);
                c.play(note_on.sample);
            }
            void operator()(const Event::SetVolume& set_vol)
            {
                c.set_volume(set_vol.volume);
            }
            Channel& c;
        };
        std::visit(ActionInterpreter{*this}, action);
    }

    bool is_active() const { return _is_active; }

    void play(const Sample* sample)
    {
        set_sample(sample);
        _sampleIndex = 0;
        _is_active = true;
    }

    void stop() { _is_active = false; }

    void set_sample(const Sample* sample_) { _sample = sample_; }

    void set_frequency(const float freq) { _frequency = freq; }

    void set_volume(const float vol) { _volume = vol; }

    void render(float* outputBuffer, unsigned long framesPerBuffer,
                const unsigned int targetSampleRate)
    {

        float rate = _frequency / static_cast<float>(targetSampleRate);
        if (!is_active() || _sample == nullptr) {
            std::memset(outputBuffer, 0,
                        framesPerBuffer * sizeof outputBuffer[0]);
            return;
        }
        while (framesPerBuffer--) {
            if (static_cast<size_t>(_sampleIndex) >= _sample->loopEnd()) {
                if (_sample->loopType() ==
                    Sample::LoopParams::Type::non_looping) {
                    // If we're done with this sample fill the rest with zeros
                    std::memset(outputBuffer, 0,
                                framesPerBuffer * sizeof(outputBuffer[0]));
                    // and leave.
                    break;
                }
                _sampleIndex -= static_cast<float>(_sample->loopLength());
            }
            *outputBuffer++ = (*_sample)[_sampleIndex] * _volume;
            _sampleIndex += rate;
        }
    }

    float frequency() const { return _frequency; }
    const Sample* sample() const { return _sample; }
    float sample_index() const { return _sampleIndex; }
    float volume() const { return _volume; }

  private:
    const Sample* _sample = nullptr;
    float _sampleIndex = 0;
    float _frequency = 1.0f;
    float _volume = 1.0f;
    bool _is_active = false;
};

extern std::ostream& operator<<(std::ostream& os,
                                const Channel::Event::Action& action);

#endif
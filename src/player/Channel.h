#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "Sample.h"

class Channel {

public:

    bool is_active() const {
        return _is_active;
    }

    void play(const Sample* sample) {
        set_sample(sample);
        _sampleIndex = 0;
        _is_active = true;
    }

    void stop() {
        _is_active = false;
    }

    void set_sample(const Sample* sample_) {
        _sample = sample_;
    }

    void set_frequency(const float freq) {
        _frequency = freq;
    }

    void set_volume(const float vol) {
        _volume = vol;
    }

    void render(float* outputBuffer, unsigned long framesPerBuffer, const unsigned int targetSampleRate) {

        float rate = _frequency / targetSampleRate;
        if (!is_active() || _sample == nullptr) {
            memset(outputBuffer, 0, framesPerBuffer * sizeof outputBuffer[0]);
            return;
        }
        while (framesPerBuffer--) {
            if (_sampleIndex >= _sample->loopEnd()) {
                if (_sample->loopType() == Sample::LoopParams::Type::non_looping) {
                    // If we're done with this sample fill the rest with zeros
                    memset(outputBuffer, 0, framesPerBuffer * sizeof(outputBuffer[0]));
                    // and leave.
                    break;
                }
                _sampleIndex -= _sample->loopLength();
            }
            *outputBuffer++ = (*_sample)[_sampleIndex] * _volume;
            _sampleIndex += rate;
        }
    }

private:
    const Sample* _sample = nullptr;
    float _sampleIndex = 0;
    float _frequency = 1.0f;
    float _volume = 1.0f;
    bool _is_active = false;
};

#endif
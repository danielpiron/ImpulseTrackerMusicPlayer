#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "Sample.h"

class Channel {

public:
    void set_sample(const Sample* sample_) {
        _sample = sample_;
        _sampleIndex = 0;
    }

    void set_frequency(const float freq) {
        _frequency = freq;
    }

    void render(float* outputBuffer, unsigned long framesPerBuffer, const unsigned int targetSampleRate) {

        float rate = _frequency / targetSampleRate;
        if (_sample == nullptr) {
            memset(outputBuffer, 0, framesPerBuffer * sizeof outputBuffer[0]);
            return;
        }
        while (framesPerBuffer--) {
            if (_sampleIndex >= _sample->loopEnd()) {
                _sampleIndex -= _sample->loopLength();
            }
            *outputBuffer++ = (*_sample)[_sampleIndex];
            _sampleIndex += rate;
        }
    }

private:
    const Sample* _sample = nullptr;
    float _sampleIndex = 0;
    float _frequency = 44100;
};

#endif
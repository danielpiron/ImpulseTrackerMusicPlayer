#include <portaudio.h>

#include <cmath>
#include <iostream>
#include <vector>

class Sample {
public:
    float operator[](const size_t index) const {
        return _samples[index];
    }
    float operator[](const float index) const {
        auto iIndex = static_cast<size_t>(floor(index));
        float t = index - iIndex;
        float v0 = _samples[iIndex];
        float v1 = iIndex == (_samples.size() - 1) ? _samples[0] : _samples[iIndex + 1];
        return v0 + t * (v1 - v0); 
    }
    size_t length() const { return _samples.size(); }

public:
    std::vector<float> _samples;
};

struct Channel {

    Channel(const Sample* sample_ = nullptr) : sample(sample_)
                                             , index(0)
                                             , rate(1.0f) {}

    void render(float* outputBuffer, unsigned long framesPerBuffer) {
        if (sample == nullptr) return;
        while (framesPerBuffer) {
            size_t samplesUntilLoop = sample->length() - static_cast<size_t>(index);
            bool loopOccurs = samplesUntilLoop < framesPerBuffer;
            size_t renderPassLength = loopOccurs ? samplesUntilLoop : framesPerBuffer;

            framesPerBuffer -= renderPassLength;
            while (renderPassLength--) {
                *outputBuffer++ = (*sample)[index];
                index += rate;
            }

            if (loopOccurs) {
                index -= sample->length();
            }
        }
    }
    const Sample* sample;
    float index;
    float rate;
};

static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    auto channel = reinterpret_cast<Channel*>(userData);
    float* pOut = reinterpret_cast<float*>(outputBuffer);
    channel->render(pOut, framesPerBuffer);

    return paContinue;
}

static void StreamFinished(void* userData)
{
    (void)userData;
    std::cout << "Stream complete" << std::endl;
}

int main() {

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << Pa_GetErrorText(err);
    }

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        std::cerr << "Error: No default output device" << std::endl;
        return 1;
    }
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    Sample sample;
    size_t sampleCount = 44100 / 440;
    for (size_t i = 0; i < sampleCount; ++i) {
        sample._samples.push_back(static_cast<float>(sin(M_PI * 2 * i / sampleCount)));
    }
    Channel channel(&sample);

    PaStream* stream = nullptr;
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        44100,
        paFramesPerBufferUnspecified,
        paClipOff,
        patestCallback,
        reinterpret_cast<void*>(&channel));
    if (err != paNoError) {
        std::cerr << "Error opening stream" << std::endl;
        return 1;
    }

    Pa_SetStreamFinishedCallback(stream, StreamFinished);
    Pa_StartStream(stream);

    std::cout << "Press any key to quit" << std::endl;
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    Pa_Terminate();
    std::cout << "Completed" << std::endl;

    return 0;
}

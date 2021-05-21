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

    struct Note {
        float frequency;
        float volume;
    };
    
    Channel(const Sample* sample_ = nullptr) : sample(sample_)
                                             , sampleIndex(0)
                                             , note({523.0f, 1.0}) {}

    void render(float* outputBuffer, unsigned long framesPerBuffer, unsigned int sampleRate=44100) {
        if (sample == nullptr) return;

        float rate = sampleRate / note.frequency;
        while (framesPerBuffer) {
            std::cout << "framesPerBuffer: " << framesPerBuffer << "\n";
            std::cout << "Sample length: " << sample->length() << "\n";
            std::cout << "Sample index: " << sampleIndex << "\n";
            std::cout << "Rate: " << rate << std::endl;
            size_t samplesUntilLoop = static_cast<size_t>((sample->length() - static_cast<size_t>(sampleIndex)) / rate);
            std::cout << "SamplesUntilLoop: " << samplesUntilLoop << std::endl;
            bool loopOccurs = samplesUntilLoop < framesPerBuffer;
            size_t renderPassLength = loopOccurs ? samplesUntilLoop : framesPerBuffer;

            framesPerBuffer -= renderPassLength;
            while (renderPassLength--) {
                auto value = (*sample)[sampleIndex];
                std::cout << value << "\n";
                *outputBuffer++ =  value;
                sampleIndex += rate;
            }

            std::cout << "samplesIndex after: " << sampleIndex << std::endl;
            if (loopOccurs) {
                sampleIndex -= sample->length();
            }
        }
    }

    const Sample* sample;
    float sampleIndex;
    Note note;
};

struct AudioGenerator {
    AudioGenerator(size_t channelCount_, unsigned int sampleRate_)
    : channels(channelCount_)
    , sampleRate(sampleRate_)
    , samplesPerFrame(22050)
    , samplesUntilNextFrame(0)
    , row(0)
    {}

    void render(float* outputBuffer, unsigned long framesPerBuffer) {

        while (framesPerBuffer) {
            if (samplesUntilNextFrame == 0) {
                channels[0].note = notes[row++ % notes.size()];
                samplesUntilNextFrame = samplesPerFrame;
            }

            auto samplesToRender = framesPerBuffer < samplesUntilNextFrame
                                   ? framesPerBuffer
                                   : samplesUntilNextFrame;

            framesPerBuffer -= samplesToRender;
            samplesUntilNextFrame -= samplesToRender;
            channels[0].render(outputBuffer, samplesToRender, sampleRate);
        }
    }

    std::vector<Channel> channels;
    std::vector<Channel::Note> notes;
    unsigned int sampleRate;
    size_t samplesPerFrame;
    size_t samplesUntilNextFrame;
    size_t row;
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

    auto audio = reinterpret_cast<AudioGenerator*>(userData);
    float* pOut = reinterpret_cast<float*>(outputBuffer);
    audio->render(pOut, framesPerBuffer);

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
    unsigned int sampleCount = 44100;
    for (size_t i = 0; i < sampleCount; ++i) {
        sample._samples.push_back(static_cast<float>(sin(M_PI * 2 * i / sampleCount)));
    }

    AudioGenerator audio(1, sampleCount);
    audio.channels[0].sample = &sample;
    audio.notes = {
           {523.25f, 1.0},  // C5
//         {554.37f, 1.0},  // C#5
           {587.33f, 1.0},  // D5
//         {622.25f, 1.0},  // D#5
           {659.25f, 1.0},  // E5
           {698.46f, 1.0},  // F5
//         {739.99f, 1.0},  // F#5
           {783.99f, 1.0},  // G5
//         {830.61f, 1.0},  // G#5
           {880.00f, 1.0},  // A5
//         {932.33f, 1.0},  // A#5
           {987.77f, 1.0},  // B5
           {1046.50f, 1.0}, // C6
    };

    PaStream* stream = nullptr;
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        44100,
        paFramesPerBufferUnspecified,
        paClipOff,
        patestCallback,
        reinterpret_cast<void*>(&audio));
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

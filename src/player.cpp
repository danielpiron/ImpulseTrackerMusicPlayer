#include <portaudio.h>

#include <player/Mixer.h>

#include <cmath>
#include <iostream>
#include <vector>

static int patestCallback(const void*, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo*,
                          PaStreamCallbackFlags, void* userData)
{
    auto mixer = reinterpret_cast<Mixer*>(userData);
    auto pOut = reinterpret_cast<float*>(outputBuffer);
    mixer->render(pOut, framesPerBuffer);

    return paContinue;
}

static void StreamFinished(void* userData)
{
    (void)userData;
    std::cout << "Stream complete" << std::endl;
}

int main()
{

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
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    struct ScalePlayer : public Mixer::TickHandler {

        static Sample generateSinewave()
        {
            std::vector<float> sampleData(400);
            for (size_t i = 0; i < sampleData.size(); ++i) {
                sampleData[i] = static_cast<float>(
                    sin(M_PI * 2.0 * static_cast<double>(i) /
                        static_cast<double>(sampleData.size())));
            }
            return Sample(sampleData.begin(), sampleData.end(), 1);
        }

        ScalePlayer(std::initializer_list<float> l)
            : sineWave(generateSinewave()), notes(l), index(0)
        {
        }

        void onAttachment(Mixer& audio) override
        {
            audio.set_samples_per_tick(static_cast<size_t>(44100 * 0.5f));
            audio.channel(0).play(&sineWave);
        }

        void onTick(Mixer& audio) override
        {
            audio.channel(0).set_frequency(
                static_cast<float>(sineWave.length()) *
                notes[index++ % notes.size()]);
        }

        Sample sineWave;
        std::vector<float> notes;
        size_t index;
    };

    Mixer mixer(44100, 1);
    ScalePlayer player{
        523.25f, // C5
                 //         554.37f,  // C#5
        587.33f, // D5
                 //         622.25f,  // D#5
        659.25f, // E5
        698.46f, // F5
                 //         739.99f,  // F#5
        783.99f, // G5
                 //         830.61f,  // G#5
        880.00f, // A5
                 //         932.33f,  // A#5
        987.77f, // B5
        1046.50f // C6
    };
    mixer.attach_handler(&player);

    PaStream* stream = nullptr;
    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100,
                        paFramesPerBufferUnspecified, paClipOff, patestCallback,
                        reinterpret_cast<void*>(&mixer));
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

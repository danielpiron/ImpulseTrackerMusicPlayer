#include <portaudio.h>

#include <player/Module.h>
#include <player/Player.h>

#include <cmath>
#include <iostream>
#include <vector>

static int patestCallback(const void*, void* outputBuffer, unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
{
    auto player = reinterpret_cast<Player*>(userData);
    auto pOut = reinterpret_cast<float*>(outputBuffer);
    player->render_audio(pOut, static_cast<int>(framesPerBuffer));

    return paContinue;
}

static void StreamFinished(void* userData)
{
    (void)userData;
    std::cout << "Stream complete" << std::endl;
}

static Module::Sample generateSinewave(int sampling_rate, float target_frequency = 523.25f)
{
    std::vector<float> sampleData(static_cast<size_t>(static_cast<float>(sampling_rate) /
                                                      static_cast<float>(target_frequency)));
    for (size_t i = 0; i < sampleData.size(); ++i) {
        sampleData[i] = static_cast<float>(
            sin(M_PI * 2.0 * static_cast<double>(i) / static_cast<double>(sampleData.size())));
    }
    return Module::Sample(
        Sample(sampleData.begin(), sampleData.end(), static_cast<size_t>(sampling_rate)));
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

    auto mod = std::make_shared<Module>();
    mod->initial_tempo = 60;
    mod->initial_speed = 6;
    mod->patternOrder = {0, 255};
    mod->patterns.resize(6, Module::Pattern(16));
    mod->samples.emplace_back(generateSinewave(22050));

    parse_pattern(R"(
        C-6 01 .. D04
        ... .. .. D00
        F-5 01 40 D03
        G-5 01 45 D00
        A-5 01 50 D00
        A#5 01 55 D00
        C-6 01 60 D00
        ... .. .. D00
        F-5 01 .. D04
        ... .. .. D00
        F-5 01 .. D00
        ... .. .. D00
        ... .. .. D09
        )",
                  mod->patterns[0]);

    Player player(mod);

    PaStream* stream = nullptr;
    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100, paFramesPerBufferUnspecified,
                        paClipOff, patestCallback, reinterpret_cast<void*>(&player));
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

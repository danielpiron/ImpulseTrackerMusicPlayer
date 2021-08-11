#include <portaudio.h>

#include <player/Module.h>
#include <player/Player.h>

#include <loader/s3m.h>

#include <cmath>
#include <iostream>
#include <vector>

static int patestCallback(const void*, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo*,
                          PaStreamCallbackFlags, void* userData)
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

    std::ifstream s3m("/Users/pironvila/Downloads/ST3/S/INSIDE.S3M",
                      s3m.binary);

    auto mod = load_s3m(s3m);
    Player player(mod);

    PaStream* stream = nullptr;
    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100,
                        paFramesPerBufferUnspecified, 0, patestCallback,
                        reinterpret_cast<void*>(&player));
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

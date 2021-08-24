#include <portaudio.h>

#include <player/Module.h>
#include <player/Player.h>

#include <loader/it.h>
#include <loader/s3m.h>

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

static std::shared_ptr<Module> load_module(const char* filename)
{
    std::ifstream fs{filename, std::ios::binary};
    char ext[5];

    const char* ptr = filename;
    while (*ptr != '.' && *ptr != '\0')
        ptr++;
    std::strncpy(ext, ptr, 4);
    for (size_t i = 0; i < strnlen(ext, 4); ++i) {
        ext[i] = static_cast<char>(std::tolower(ext[i]));
    }

    if (strncmp(ext, ".s3m", 4) == 0) {
        return load_s3m(fs);
    } else if (strncmp(ext, ".it", 4) == 0) {
        return load_it(fs);
    } else {
        return std::make_shared<Module>();
    }
}

int main(int argc, char* argv[])
{

    if (argc < 2) {
        std::cout << "S3M filename please" << std::endl;
        exit(1);
    }

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
        Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    Player player(load_module(argv[1]));

    PaStream* stream = nullptr;
    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100, paFramesPerBufferUnspecified, 0,
                        patestCallback, reinterpret_cast<void*>(&player));
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

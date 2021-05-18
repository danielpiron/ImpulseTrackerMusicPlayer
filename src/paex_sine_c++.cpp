#include <portaudio.h>

#include <iostream>

static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    float* pOut = reinterpret_cast<float*>(outputBuffer);
    while (framesPerBuffer--) {
        *pOut++ = 0;
    }
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
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaStream* stream = nullptr;
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        44100,
        paFramesPerBufferUnspecified,
        paClipOff,
        patestCallback,
        NULL);
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

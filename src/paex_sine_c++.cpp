#include <portaudio.h>

#include "Channel.h"

#include <cmath>
#include <iostream>
#include <vector>

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
    auto pOut = reinterpret_cast<float*>(outputBuffer);
    channel->render(pOut, framesPerBuffer, 44100);

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

    unsigned int sampleCount = 400;
    std::vector<float> sampleData(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        sampleData[i] = static_cast<float>(sin(M_PI * 2 * i / sampleCount));
    }

    Sample sample(sampleData.begin(), sampleData.end());
    Channel channel;
    channel.set_sample(&sample);
    channel.set_frequency(523.25f * sampleCount);

/*
    notes = {
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
    */

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

    int note = 0;
    float notes[] =  {
           523.25f,  // C5
//         554.37f,  // C#5
           587.33f,  // D5
//         622.25f,  // D#5
           659.25f,  // E5
           698.46f,  // F5
//         739.99f,  // F#5
           783.99f,  // G5
//         830.61f,  // G#5
           880.00f,  // A5
//         932.33f,  // A#5
           987.77f,  // B5
           1046.50f // C6
    };

    while (true) {
        channel.set_frequency(sampleCount * notes[note++ % 8]);
        Pa_Sleep(500);
    }

    std::cout << "Press any key to quit" << std::endl;
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    Pa_Terminate();
    std::cout << "Completed" << std::endl;

    return 0;
}

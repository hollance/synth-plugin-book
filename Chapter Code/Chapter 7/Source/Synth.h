#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "NoiseGenerator.h"

class Synth
{
public:
    Synth();

    void allocateResources(double sampleRate, int samplesPerBlock);
    void deallocateResources();
    void reset();
    void render(float** outputBuffers, int sampleCount);
    void midiMessage(uint8_t data0, uint8_t data1, uint8_t data2);

    float noiseMix;

private:
    void noteOn(int note, int velocity);
    void noteOff(int note);

    float sampleRate;
    Voice voice;
    NoiseGenerator noiseGen;
};

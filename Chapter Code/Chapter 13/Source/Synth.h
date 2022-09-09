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

    float envAttack, envDecay, envSustain, envRelease;

    float oscMix;
    float detune;
    float tune;

    static constexpr int MAX_VOICES = 8;
    int numVoices;

    float volumeTrim;

    juce::LinearSmoothedValue<float> outputLevelSmoother;

    float velocitySensitivity;
    bool ignoreVelocity;

    const int LFO_MAX = 32;
    float lfoInc;

    float vibrato;
    float pwmDepth;

    int glideMode;
    float glideRate;
    float glideBend;

    float filterKeyTracking;
    float filterQ;
    float filterLFODepth;
    float filterAttack, filterDecay, filterSustain, filterRelease;
    float filterEnvDepth;

    uint8_t resoCC = 0x47;

private:
    void updateLFO();

    void controlChange(uint8_t data1, uint8_t data2);

    void noteOn(int note, int velocity);
    void noteOff(int note);

    void startVoice(int v, int note, int velocity);
    void restartMonoVoice(int note, int velocity);

    float calcPeriod(int v, int note) const;
    int findFreeVoice() const;

    void shiftQueuedNotes();
    int nextQueuedNote();

    inline void updatePeriod(Voice& voice)
    {
        voice.osc1.period = voice.period * pitchBend;
        voice.osc2.period = voice.osc1.period * detune;
    }

    bool isPlayingLegatoStyle() const;

    float sampleRate;
    std::array<Voice, MAX_VOICES> voices;
    NoiseGenerator noiseGen;

    int lastNote;

    int lfoStep;
    float lfo;
    float filterZip;

    float pitchBend;
    bool sustainPedalPressed;
    float modWheel;
    float resonanceCtl;
    float pressure;
    float filterCtl;
};

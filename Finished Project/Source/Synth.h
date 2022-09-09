#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "NoiseGenerator.h"

// The main class for the synthesizer.
class Synth
{
public:
    Synth();

    void allocateResources(double sampleRate, int samplesPerBlock);
    void deallocateResources();
    void reset();
    void render(float** outputBuffers, int sampleCount);
    void midiMessage(uint8_t data0, uint8_t data1, uint8_t data2);

    // === Parameter values ===

    // Gain for mixing noise into the output.
    float noiseMix;

    // Amplitude ADSR settings.
    float envAttack, envDecay, envSustain, envRelease;

    // How much oscillator 2 is mixed into the sound. 0.0 = osc2 is silent,
    // 1.0 = osc2 has same level as osc1. Note that osc2 is subtracted, so if
    // it is not detuned from osc1, they cancel each other out into silence.
    float oscMix;

    // Amount of detuning for oscillator 2. This is a multiplier for the period
    // of the oscillator.
    float detune;

    // Master tuning.
    float tune;

    // Max polyphony.
    static constexpr int MAX_VOICES = 8;

    // Mono (= 1 voice) / poly mode.
    int numVoices;

    // Used to keep the output gain constant after changing parameters.
    float volumeTrim;

    // Output gain.
    juce::LinearSmoothedValue<float> outputLevelSmoother;

    // Used to set the low-pass filter's cutoff frequency based on the note's
    // velocity. There is no velocity sensitivity for the amplitude envelope,
    // only for the filter cutoff.
    float velocitySensitivity;

    // If this is set, all notes will be played with the same velocity.
    bool ignoreVelocity;

    // How often the LFO and other modulations are updated, in samples.
    const int LFO_MAX = 32;

    // Phase increment for the LFO.
    float lfoInc;

    // LFO intensity for vibrato and PWM.
    float vibrato;
    float pwmDepth;

    // Glide mode: 0 = off, 1 = legato-style playing, 2 = always.
    int glideMode;

    // Coefficient for the speed of the glide. 1.0 is instantaneous (no glide).
    float glideRate;

    // Number of semitones to glide up or down into any new note. This is used
    // even if the glide mode is set to off.
    float glideBend;

    // The user does not manually set the filter's cutoff frequency, this is
    // determined by the note's pitch and velocity. This variable is used as
    // a multiplier that shifts the cutoff up or down.
    float filterKeyTracking;

    // Resonance setting for the low-pass filter.
    float filterQ;

    // LFO intensity for the filter cutoff.
    float filterLFODepth;

    // Filter ADSR settings.
    float filterAttack, filterDecay, filterSustain, filterRelease;

    // Envelope intensity for the filter cutoff.
    float filterEnvDepth;

private:
    // Performs the LFO update very 32 samples.
    void updateLFO();

    // Handles a MIDI CC event.
    void controlChange(uint8_t data1, uint8_t data2);

    // Handles a MIDI note on event.
    void noteOn(int note, int velocity);

    // Handles a MIDI note off event.
    void noteOff(int note);

    // Helper functions that set up a voice to play a new note.
    void startVoice(int v, int note, int velocity);
    void restartMonoVoice(int note, int velocity);

    // Calculate the oscillator period based on the MIDI note number.
    float calcPeriod(int v, int note) const;

    // Find a voice to use in polyphonic mode.
    int findFreeVoice() const;

    // For note queuing in monophonic mode.
    void shiftQueuedNotes();
    int nextQueuedNote();

    inline void updatePeriod(Voice& voice)
    {
        voice.osc1.period = voice.period * pitchBend;
        voice.osc2.period = voice.osc1.period * detune;
    }

    // Is at least one key still held down for any of the playing voices?
    bool isPlayingLegatoStyle() const;

    // The current sample rate.
    float sampleRate;

    // List of the active voices.
    std::array<Voice, MAX_VOICES> voices;

    // Pseudo random noise generator.
    NoiseGenerator noiseGen;

    // Most recent note that was played. Used for gliding.
    int lastNote;

    // === Modulation ===

    // The LFO only updates every 32 samples. This counter keeps track of when
    // the next update is.
    int lfoStep;

    // Current LFO value.
    float lfo;

    // Used to smoothen changes in the amount of low-pass filter modulation.
    float filterZip;

    // === MIDI CC values ===

    // Current value for the pitch bend wheel.
    float pitchBend;

    // Status of the damper pedal: true = pressed, false = released.
    bool sustainPedalPressed;

    // Modulation wheel value. Sets the modulation depth for vibrato / PWM.
    float modWheel;

    // MIDI CC amount used to modulate the filter Q.
    float resonanceCtl;

    // Amount of channel aftertouch. Used to modulate the filter cutoff.
    float pressure;

    // MIDI CC amount used to modulate the cutoff frequency.
    float filterCtl;
};

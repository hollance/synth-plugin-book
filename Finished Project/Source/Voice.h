#pragma once

#include "Oscillator.h"
#include "Envelope.h"
#include "Filter.h"

// State for an active voice.
struct Voice
{
    // The MIDI note number that this voice is playing, or the special value
    // SUSTAIN when the key has been released but the sustain pedal is held
    // down. Is 0 if the voice is inactive.
    int note;

    // The current period of the waveform in samples, which may be gliding up
    // to the value from `target`.
    float period;

    // The desired period in samples.
    float target;

    // Oscillators
    Oscillator osc1;
    Oscillator osc2;

    // Integrates the outputs from the oscillators to produce a sawtooth wave.
    float saw;

    // Amplitude envelope.
    Envelope env;

    // Filter and its envelope.
    Filter filter;
    Envelope filterEnv;

    // The filter's base cutoff frequency based on pitch and velocity, in Hz.
    float cutoff;

    // The filter resonance.
    float filterQ;

    // Modulation value that is computed by Synth but that Voice needs.
    float filterMod;

    // The synth parameters and MIDI controller values this voice needs.
    float glideRate;
    float pitchBend;
    float filterEnvDepth;

    // Panning amounts for left and right channels.
    float panLeft, panRight;

    void reset()
    {
        note = 0;
        saw = 0.0f;

        osc1.reset();
        osc2.reset();
        env.reset();
        filterEnv.reset();
        filter.reset();

        panLeft = 0.707f;
        panRight = 0.707f;
    }

    float render(float input)
    {
        // The two oscillators output a bandlimited impulse train, which
        // consists of a sinc pulse every `period` samples.
        float sample1 = osc1.nextSample();
        float sample2 = osc2.nextSample();

        // By adding up the sinc pulses over time, i.e. by integrating them,
        // this creates a bandlimited sawtooth wave without much aliasing.
        // Subtracting the osc2 sawtooth from osc1 creates a square wave.
        // For the best results, osc2 should be detuned otherwise it will
        // cancel out with osc1 and give silence.
        saw = saw * 0.997f + sample1 - sample2;

        // Note: It can be a little unpredictable how these two oscillators
        // interact. The oscillator state is not reset when an old voice is
        // reused for a new note, and so the phase difference between osc1
        // and osc2 is never the same -- which is part of the fun.

        // Combine the output from the oscillators with the noise.
        float output = saw + input;

        // Apply the resonant low-pass filter.
        output = filter.render(output);

        // Amplitude envelope.
        float envelope = env.nextValue();

        // The output for this voice is the amplitude envelope times the
        // output from the filter.
        return output * envelope;
    }

    void updatePanning()
    {
        // Put middle C (note 60) in the center of the stereo field.
        // Fully panned left is note (60 - 24), fully right is note (60 + 24).
        float panning = std::clamp((note - 60.0f) / 24.0f, -1.0f, 1.0f);

        // Use constant power panning formula.
        panLeft = std::sin(PI_OVER_4 * (1.0f - panning));
        panRight = std::sin(PI_OVER_4 * (1.0f + panning));
    }

    void updateLFO()
    {
        // Do the following updates at the LFO update rate.

        // Glide between pitches using a simple one-pole smoothing filter.
        period += glideRate * (target - period);

        // Update the filter envelope. This is the same equation as for the
        // amplitude envelope, but only performed every LFO_MAX steps.
        float fenv = filterEnv.nextValue();

        // Calculate the filter cutoff frequency. The base `cutoff` is given by
        // the pitch and velocity. This is modulated by a variety of other things
        // such as the filter envelope and the pitch bend.
        float modulatedCutoff = cutoff * std::exp(filterMod + filterEnvDepth * fenv) / pitchBend;

        // Make sure the cutoff frequency stays within reasonable bounds.
        modulatedCutoff = std::clamp(modulatedCutoff, 30.0f, 20000.0f);

        // Tell the filter to recalculate its coefficients.
        filter.updateCoefficients(modulatedCutoff, filterQ);
    }

    void release()
    {
        env.release();
        filterEnv.release();
    }
};

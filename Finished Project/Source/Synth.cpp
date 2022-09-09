#include "Synth.h"
#include "Utils.h"

static const float ANALOG = 0.002f;   // oscillator drift

// Special "note number" that says this voice is now kept alive by the sustain
// pedal being pressed down. As soon as the pedal is released, this voice will
// fade out.
static const int SUSTAIN = -1;

Synth::Synth()
{
    sampleRate = 44100.0f;
}

void Synth::allocateResources(double sampleRate_, int /*samplesPerBlock*/)
{
    sampleRate = static_cast<float>(sampleRate_);

    // For using the JUCE LadderFilter:
    //juce::dsp::ProcessSpec spec;
    //spec.sampleRate = sampleRate;
    //spec.maximumBlockSize = samplesPerBlock;
    //spec.numChannels = 1;

    for (int v = 0; v < MAX_VOICES; ++v) {
        voices[v].filter.sampleRate = sampleRate;

        // For using the JUCE LadderFilter:
        //voices[v].filter.setMode(juce::dsp::LadderFilterMode::LPF12);
        //voices[v].filter.prepare(spec);
    }
}

void Synth::deallocateResources()
{
    // do nothing
}

void Synth::reset()
{
    // Turn off all playing voices.
    for (int v = 0; v < MAX_VOICES; ++v) {
        voices[v].reset();
    }

    noiseGen.reset();

    // These variables are changed by MIDI CC, reset to defaults.
    pitchBend = 1.0f;
    sustainPedalPressed = false;
    modWheel = 0.0f;
    resonanceCtl = 1.0f;
    pressure = 0.0f;
    filterCtl = 0.0f;

    // Reset other state.
    lfo = 0.0f;
    lfoStep = 0;
    lastNote = 0;
    filterZip = 0.0f;

    outputLevelSmoother.reset(sampleRate, 0.05);
}

void Synth::render(float** outputBuffers, int sampleCount)
{
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];

    // The voices need to have access to some of the synth's parameters and
    // MIDI controller values. We copy these values into the active voices
    // at the start of the block. They will never change during the block.
    for (int v = 0; v < MAX_VOICES; ++v) {
        Voice& voice = voices[v];
        if (voice.env.isActive()) {
            updatePeriod(voice);
            voice.glideRate = glideRate;
            voice.filterQ = filterQ * resonanceCtl;
            voice.pitchBend = pitchBend;
            voice.filterEnvDepth = filterEnvDepth;
        }
    }

    for (int sample = 0; sample < sampleCount; ++sample) {

        // The LFO and any things it modulates are updated every 32 samples.
        // It's also guaranteed to be called the very first time.
        updateLFO();

        // Noise oscillator.
        float noise = noiseGen.nextValue() * noiseMix;

        // These variables add up the output values of all the active voices.
        float outputLeft = 0.0f;
        float outputRight = 0.0f;

        // Render the voices that have an active envelope.
        for (int v = 0; v < MAX_VOICES; ++v) {
            Voice& voice = voices[v];
            if (voice.env.isActive()) {
                float output = voice.render(noise);
                outputLeft += output * voice.panLeft;
                outputRight += output * voice.panRight;
            }
        }

        // Apply additional gain.
        float outputLevel = outputLevelSmoother.getNextValue();
        outputLeft *= outputLevel;
        outputRight *= outputLevel;

        // Write the result into the output buffer.
        if (outputBufferRight != nullptr) {
            outputBufferLeft[sample] = outputLeft;
            outputBufferRight[sample] = outputRight;
        } else {
            outputBufferLeft[sample] = (outputLeft + outputRight) * 0.5f;
        }
    }

    // Turn off voices whose envelope has dropped below the minimum level.
    for (int v = 0; v < MAX_VOICES; ++v) {
        Voice& voice = voices[v];
        if (!voice.env.isActive()) {
            voice.env.reset();
            voice.filter.reset();
        }
    }

    protectYourEars(outputBufferLeft, sampleCount);
    protectYourEars(outputBufferRight, sampleCount);
}

void Synth::updateLFO()
{
    if (--lfoStep <= 0) {
        lfoStep = LFO_MAX;  // reset the counter

        lfo += lfoInc;
        if (lfo > PI) { lfo -= TWO_PI; }

        // The LFO is a basic sine wave.
        const float sine = std::sin(lfo);

        // The modulation intensity for vibrato / PWM is set by the parameter
        // and by the modulation wheel. Together, they can modulate the pitch
        // by approximately two semitones up and down.
        float vibratoMod = 1.0f + sine * (modWheel + vibrato);
        float pwm = 1.0f + sine * (modWheel + pwmDepth);

        // The low-pass filter cutoff is modulated by the combination of the
        // Filter Freq parameter set by the user, the MIDI CC, aftertouch, and
        // the LFO intensity. This value swings between approx -7.97 and 11.7.
        // The Voice will also add the filter envelope to this.
        float filterMod = filterKeyTracking + filterCtl + (filterLFODepth + pressure) * sine;

        // Use a basic one-pole smoothing filter to de-zipper changes to the
        // amount of filter modulation.
        filterZip += 0.005f * (filterMod - filterZip);

        // Tell all active voices to perform any computations that depend on
        // the LFO modulations.
        for (int v = 0; v < MAX_VOICES; ++v) {
            Voice& voice = voices[v];
            if (voice.env.isActive()) {
                voice.osc1.modulation = vibratoMod;
                voice.osc2.modulation = pwm;
                voice.filterMod = filterZip;
                voice.updateLFO();
                updatePeriod(voice);
            }
        }
    }
}

void Synth::midiMessage(uint8_t data0, uint8_t data1, uint8_t data2)
{
    switch (data0 & 0xF0) {  // status byte (all channels)
        // Note off
        case 0x80:
            noteOff(data1 & 0x7F);
            break;

        // Note on
        case 0x90: {
            uint8_t note = data1 & 0x7F;
            uint8_t velo = data2 & 0x7F;
            if (velo > 0) {
                noteOn(note, velo);
            } else {
                noteOff(note);
            }
            break;
        }

        // Control change
        case 0xB0:
            controlChange(data1, data2);
            break;

        // Channel aftertouch
        case 0xD0:
            // This maps the pressure value to a parabolic curve starting at
            // 0.0 (position 0) up to 1.61 (position 127).
            pressure = 0.0001f * float(data1 * data1);
            break;

        // Pitch bend
        case 0xE0:
            // The pitch wheel can shift the tone up or down by 2 semitones.
            pitchBend = std::exp(-0.000014102f * float(data1 + 128 * data2 - 8192));
            break;
    }
}

void Synth::controlChange(uint8_t data1, uint8_t data2)
{
    switch (data1) {
        // Mod wheel
        case 0x01:
            modWheel = 0.000005f * float(data2 * data2);
            break;

        // Sustain pedal
        case 0x40:
            sustainPedalPressed = (data2 >= 64);

            // Pedal released? Then end all sustained notes. This sends a
            // note-off event with note = -1, meaning all sustained notes
            // will be moved into their envelope release stage.
            if (!sustainPedalPressed) {
                noteOff(SUSTAIN);
            }
            break;

        // Resonance
        case 0x47:
        case 0x17:  // knob on my MIDI controller
            resonanceCtl = 154.0f / float(154 - data2);
            break;

        // Filter +
        case 0x4A:
        case 0x15:  // knob on my MIDI controller
            filterCtl = 0.02f * float(data2);
            break;

        // Filter -
        case 0x4B:
        case 0x16:  // knob on my MIDI controller
            filterCtl = -0.03f * float(data2);
            break;

        // All notes off
        default:
            if (data1 >= 0x78) {
                for (int v = 0; v < MAX_VOICES; ++v) {
                    voices[v].reset();
                }
                sustainPedalPressed = false;
            }
            break;
    }
}

void Synth::noteOn(int note, int velocity)
{
    if (ignoreVelocity) { velocity = 80; }

    int v = 0;  // index of the voice to use (0 = mono voice)

    if (numVoices == 1) {  // monophonic
        if (voices[0].note > 0) {  // legato-style playing
            shiftQueuedNotes();
            restartMonoVoice(note, velocity);
            return;
        }
    } else {  // polyphonic
        v = findFreeVoice();
    }

    startVoice(v, note, velocity);
}

void Synth::noteOff(int note)
{
    // In monophonic mode and the currently playing note is released?
    if ((numVoices == 1) && (voices[0].note == note)) {
        // Did we find an older note whose key is still held down?
        int queuedNote = nextQueuedNote();
        if (queuedNote > 0) {
            // Put this note into voice 0 and restart it.
            restartMonoVoice(queuedNote, -1);
        }
    }

    // We get here in polyphonic mode, or when a key was released that is
    // not currently playing in monophonic mode.
    // We also get here when the sustain pedal is released. In that case,
    // the note number is -1 (SUSTAIN).

    for (int v = 0; v < MAX_VOICES; v++) {
        // Any voices playing this note?
        if (voices[v].note == note) {
            if (sustainPedalPressed) {
                // Sustain pedal is pressed, so put the note in sustain mode.
                voices[v].note = SUSTAIN;
            } else {
                // Sustain pedal is not pressed, so start envelope release.
                voices[v].release();
                voices[v].note = 0;
            }
        }
    }
}

void Synth::startVoice(int v, int note, int velocity)
{
    float period = calcPeriod(v, note);

    // Set the period as the target that we'll glide to (if glide enabled).
    Voice& voice = voices[v];
    voice.target = period;

    // Determine if we need to perform a portamento from the previous note's
    // pitch to the new one. Note that legato-style playing in monophonic mode
    // is handled elsewhere.
    int noteDistance = 0;
    if (lastNote > 0) {
        if ((glideMode == 2) || ((glideMode == 1) && isPlayingLegatoStyle())) {
            noteDistance = note - lastNote;
        }
    }

    // If gliding, make the starting period equal to the period of the previous
    // note. Also offset it by an additional amount of glide bending, given in
    // semitones. `glideBend` is always used, even if gliding is disabled.
    voice.period = period * std::pow(1.059463094359f, float(noteDistance) - glideBend);

    // Make sure the starting period does not become too small. Unlike the
    // target period, this doesn't need to be exact, so we can simply limit
    // it to the minimum of 6 samples.
    if (voice.period < 6.0f) { voice.period = 6.0f; }

    // Remember which note was last played, for gliding next time.
    lastNote = note;
    voice.note = note;
    voice.updatePanning();

    // Set the base cutoff frequency for the low-pass filter, based on the
    // pitch of the note and its velocity.
    voice.cutoff = sampleRate / (period * PI);
    voice.cutoff *= std::exp(velocitySensitivity * float(velocity - 64));

    // The loudness of the tone uses the MIDI velocity but you cannot set the
    // sensitivity other than on/off. Convert the linear velocity into a curve
    // that is parabolic.
    float vel = 0.004f * float((velocity + 64) * (velocity + 64)) - 8.0f;

    // Use the different volume controls to set the amplitude level (a value
    // between 0 and 1) for both oscillators.
    voice.osc1.amplitude = volumeTrim * vel;
    voice.osc2.amplitude = voice.osc1.amplitude * oscMix;

    // OPTIONAL: reset the oscillators.
    //voice.osc1.reset();
    //voice.osc2.reset();

    // In PWM mode, change the starting phase of the second oscillator so that
    // it combines with the first oscillator into a square wave.
    if (vibrato == 0.0f && pwmDepth > 0.0f) {
        voice.osc2.squareWave(voice.osc1, voice.period);
    }

    // Set the parameters for the envelope and start the attack.
    Envelope& env = voice.env;
    env.attackMultiplier = envAttack;
    env.decayMultiplier = envDecay;
    env.sustainLevel = envSustain;
    env.releaseMultiplier = envRelease;
    env.attack();

    Envelope& filterEnv = voice.filterEnv;
    filterEnv.attackMultiplier = filterAttack;
    filterEnv.decayMultiplier = filterDecay;
    filterEnv.sustainLevel = filterSustain;
    filterEnv.releaseMultiplier = filterRelease;
    filterEnv.attack();
}

void Synth::restartMonoVoice(int note, int velocity)
{
    // This is a simplified version of startVoice, used only in mono mode when
    // playing legato-style or when activating a queued note after a key up.

    float period = calcPeriod(0, note);

    Voice& voice = voices[0];
    voice.target = period;

    // Glide mode is off? Then no portamento. Otherwise, glide from whatever
    // was the previous period for this voice. Note that this does not use the
    // additional glide bend parameter.
    if (glideMode == 0) { voice.period = period; }

    // Same formula as in startVoice. When playing a queued note we do not have
    // the velocity anymore, so just ignore that part when setting the low-pass
    // filter cutoff.
    voice.cutoff = sampleRate / (period * PI);
    if (velocity > 0) {
        voice.cutoff *= std::exp(velocitySensitivity * float(velocity - 64));
    }

    voice.env.level += SILENCE + SILENCE;
    voice.note = note;
    voice.updatePanning();
}

float Synth::calcPeriod(int v, int note) const
{
    // Calculate the period in samples. This formula may look complicated but
    // is explained in detail in the book.
    // The ANALOG term adds a small amount of detuning based on the current
    // voice number. For moar analog!
    float period = tune * std::exp(-0.05776226505f * (float(note) + ANALOG * float(v)));

    // Make sure the period does not become too small. This lowers the pitch an
    // octave at a time until `period` is at least six samples long.
    while (period < 6.0f || (period * detune) < 6.0f) { period += period; }

    return period;
}

int Synth::findFreeVoice() const
{
    int v = 0;
    float l = 100.0f;  // louder than any envelope!

    for (int i = 0; i < MAX_VOICES; ++i) {
        // Replace quietest voice not in attack. This will first use any voices
        // that are not playing (with env level = 0.0). If all are in use, pick
        // the voice with the lowest envelope value.
        if (voices[i].env.level < l && !voices[i].env.isInAttack()) {
            l = voices[i].env.level;
            v = i;
        }
    }
    return v;
}

void Synth::shiftQueuedNotes()
{
    // Queue any held notes. This puts the previous note numbers into the other
    // Voice objects, but it won't actually play these voices. Used during the
    // next Note Off event to determine which note to restore.
    for (int tmp = MAX_VOICES - 1; tmp > 0; tmp--) {
        voices[tmp].note = voices[tmp - 1].note;

        // Edge case: the user is playing multiple notes in polyphonic mode
        // and switches to monophonic mode. We then need to fade out all the
        // other voices or they would keep ringing forever.
        voices[tmp].release();
    }
}

int Synth::nextQueuedNote()
{
    // Are there any older notes queued? Note that some of these may have
    // been released in the mean time, in which case `voice.note` was set
    // to 0 or SUSTAIN (in the loop from the else clause below). This means
    // notes kept alive only by the sustain pedal are not restored.
    int held = 0;
    for (int v = MAX_VOICES - 1; v > 0; v--) {
        if (voices[v].note > 0) { held = v; }
    }

    // Remove this older note from the queue.
    if (held > 0) {
        int note = voices[held].note;
        voices[held].note = 0;
        return note;
    }

    // No notes in the queue.
    return 0;
}

bool Synth::isPlayingLegatoStyle() const
{
    int held = 0;
    for (int i = 0; i < MAX_VOICES; ++i) {
        // Count how many playing voices are for keys that are still held down,
        // i.e. that did not get a Note Off event yet. If note is 0, this voice
        // is not playing; if it's -1, the note is sustained by the pedal.
        if (voices[i].note > 0) { held += 1; }
    }
    return held > 0;
}

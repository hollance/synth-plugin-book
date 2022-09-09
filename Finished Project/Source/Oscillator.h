#pragma once

#include <cmath>

const float PI_OVER_4 = 0.7853981633974483f;
const float PI = 3.1415926535897932f;
const float TWO_PI = 6.2831853071795864f;

// Bandlimited impulse train (BLIT) oscillator.
class Oscillator
{
public:
    // The new period in samples. Won't take effect until the next cycle.
    float period = 0.0f;

    // Modulations to be applied to the period. 1.0 = no modulation.
    float modulation = 1.0f;

    // Output level for this oscillator.
    float amplitude = 1.0f;

    void reset()
    {
        inc = 0.0f;
        phase = 0.0f;
        sin0 = 0.0f;
        sin1 = 0.0f;
        dsin = 0.0f;
        dc = 0.0f;
    }

    // Creates a sinc pulse every `period` samples.
    float nextSample()
    {
        float output = 0.0f;

        phase += inc;  // increment position in time

        if (phase <= PI_OVER_4) {
            // This is executed the very first time and after every cycle.

            // Set the period for the next cycle. Even though the period can be
            // modulated (vibrato, pitch bend, glide), it's only changed on the
            // start of the next cycle, never in the middle of an ongoing cycle.
            float halfPeriod = (period / 2.0f) * modulation;

            // Calculate the halfway point between this peak and the next,
            // expressed in samples.
            phaseMax = std::floor(0.5f + halfPeriod) - 0.5f;

            // The DC offset is necessary for turning the impulse train into a
            // sawtooth wave. The total DC offset for one cycle of the sawtooth
            // is half the amplitude. Divide that by the number of samples to
            // get the DC offset per sample.
            dc = 0.5f * amplitude / phaseMax;

            // The sinc function is sin(phase * PI) / (phase * PI), so to avoid
            // having to multiply by PI all the time, the unit of the phase and
            // therefore phaseMax and inc variables is "samples times PI".
            phaseMax *= PI;

            // In theory, the phase increment `inc` is equal to PI, except the
            // halfway point has been "fudged" a little to help reduce aliasing,
            // so `inc` will not be exactly PI (but close to it).
            inc = phaseMax / halfPeriod;

            // After the halfway point, the phase counts down to the next peak.
            // Once we're at the peak (now), we'll make the phase go up again.
            phase = -phase;

            // Initialize the sine oscillator.
            sin0 = amplitude * std::sin(phase);
            sin1 = amplitude * std::sin(phase - inc);
            dsin = 2.0f * std::cos(inc);

            // Output the peak of the sinc pulse. Make sure to not divide by 0.
            if (phase*phase > 1e-9) {
                output = sin0 / phase;
            } else {
                output = amplitude;
            }
        } else {
            // Crossed the halfway point? Then do the second half of the sinc
            // pulse in reverse, counting backwards until the next peak.
            if (phase > phaseMax) {
                phase = phaseMax + phaseMax - phase;
                inc = -inc;
            }

            // Sine wave approximation.
            float sinp = dsin * sin0 - sin1;
            sin1 = sin0;
            sin0 = sinp;

            // Sinc function: y = sin(x) / x.
            output = sinp / phase;
        }

        // Return the value minus the DC offset.
        return output - dc;
    }

    void squareWave(Oscillator& other, float newPeriod)
    {
        reset();

        // Normally the two oscillators have their own independent phase that
        // is never "synced up" anywhere. However, to make a square wave, the
        // negative peak from the second oscillator should fall somewhere in
        // between two positive peaks from the first oscillator. To do this,
        // we explicitly set the phase of the second oscillator to the phase
        // of the first, but shifted by half a cycle.

        if (other.inc > 0.0f) {
            phase = other.phaseMax + other.phaseMax - other.phase;
            inc = -other.inc;
        } else if (other.inc < 0.0f) {
            phase = other.phase;
            inc = other.inc;
        } else {
            // The other oscillator has not started yet so its phase increment
            // is still zero. Usually `inc` is around PI, so just pick that.
            phase = -PI;
            inc = PI;
        }

        // Shift by 180 degrees relative to the other sawtooth wave.
        phase += PI * newPeriod / 2.0f;
        phaseMax = phase;
    }

private:
    // Current phase, in samples times PI.
    float phase;

    // The phase counts up to this value...
    float phaseMax;

    // ...by this increment.
    float inc;

    // Direct form sine oscillator.
    float sin0;
    float sin1;
    float dsin;

    // DC offset. This is subtracted to create the sawtooth wave.
    float dc;
};

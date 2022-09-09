#pragma once

#include <cmath>

const float PI_OVER_4 = 0.7853981633974483f;
const float PI = 3.1415926535897932f;
const float TWO_PI = 6.2831853071795864f;

class Oscillator
{
public:
    float period = 0.0f;
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

    float nextSample()
    {
        float output = 0.0f;

        phase += inc;
        if (phase <= PI_OVER_4) {
            float halfPeriod = period / 2.0f;
            phaseMax = std::floor(0.5f + halfPeriod) - 0.5f;

            dc = 0.5f * amplitude / phaseMax;
            phaseMax *= PI;

            inc = phaseMax / halfPeriod;
            phase = -phase;

            sin0 = amplitude * std::sin(phase);
            sin1 = amplitude * std::sin(phase - inc);
            dsin = 2.0f * std::cos(inc);

            if (phase*phase > 1e-9) {
                output = sin0 / phase;
            } else {
                output = amplitude;
            }
        } else {
            if (phase > phaseMax) {
                phase = phaseMax + phaseMax - phase;
                inc = -inc;
            }

            float sinp = dsin * sin0 - sin1;
            sin1 = sin0;
            sin0 = sinp;

            output = sinp / phase;
        }

        return output - dc;
    }

private:
    float phase;
    float phaseMax;
    float inc;

    float sin0;
    float sin1;
    float dsin;

    float dc;
};

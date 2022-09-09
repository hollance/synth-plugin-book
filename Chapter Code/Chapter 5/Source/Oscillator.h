#pragma once

const float TWO_PI = 6.2831853071795864f;

class Oscillator
{
public:
    float amplitude;
    float inc;
    float phase;

    void reset()
    {
        phase = 0.0f;
        sin0 = amplitude * std::sin(phase * TWO_PI);
        sin1 = amplitude * std::sin((phase - inc) * TWO_PI);
        dsin = 2.0f * std::cos(inc * TWO_PI);
    }

    float nextSample()
    {
        float sinx = dsin * sin0 - sin1;
        sin1 = sin0;
        sin0 = sinx;
        return sinx;
    }

private:
    float sin0;
    float sin1;
    float dsin;
};

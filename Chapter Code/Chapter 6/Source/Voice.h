#pragma once

#include "Oscillator.h"

struct Voice
{
    int note;
    float saw;
    Oscillator osc;

    void reset()
    {
        note = 0;
        saw = 0.0f;
        osc.reset();
    }

    float render()
    {
        float sample = osc.nextSample();
        saw = saw * 0.997f + sample;
        return saw;
    }
};

#pragma once

struct Voice
{
    int note;
    int velocity;

    void reset()
    {
        note = 0;
        velocity = 0;
    }
};

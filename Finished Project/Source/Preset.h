#pragma once

#include <cstring>

const int NUM_PARAMS = 26;

// Describes a factory preset.
struct Preset
{
    Preset(const char* name,
           float p0,  float p1,  float p2,  float p3,
           float p4,  float p5,  float p6,  float p7,
           float p8,  float p9,  float p10, float p11,
           float p12, float p13, float p14, float p15,
           float p16, float p17, float p18, float p19,
           float p20, float p21, float p22, float p23,
           float p24, float p25)
    {
        strcpy(this->name, name);
        param[0]  = p0;   // Osc Mix
        param[1]  = p1;   // Osc Tune
        param[2]  = p2;   // Osc Fine
        param[3]  = p3;   // Glide Mode
        param[4]  = p4;   // Glide Rate
        param[5]  = p5;   // Glide Bend
        param[6]  = p6;   // Filter Freq
        param[7]  = p7;   // Filter Reso
        param[8]  = p8;   // Filter Env
        param[9]  = p9;   // Filter LFO
        param[10] = p10;  // Velocity
        param[11] = p11;  // Filter Attack
        param[12] = p12;  // Filter Decay
        param[13] = p13;  // Filter Sustain
        param[14] = p14;  // Filter Release
        param[15] = p15;  // Env Attack
        param[16] = p16;  // Env Decay
        param[17] = p17;  // Env Sustain
        param[18] = p18;  // Env Release
        param[19] = p19;  // LFO Rate
        param[20] = p20;  // Vibrato
        param[21] = p21;  // Noise
        param[22] = p22;  // Octave
        param[23] = p23;  // Tuning
        param[24] = p24;  // Output Level
        param[25] = p25;  // Polyphony
    }

    char name[40];
    float param[NUM_PARAMS];
};

#pragma once

const float SILENCE = 0.0001f;  // voice choking

// Analog style envelope generator.
class Envelope
{
public:
    void reset()
    {
        level = 0.0f;
        target = 0.0f;
        multiplier = 0.0f;
    }

    float nextValue()
    {
        // Update the amplitude envelope. This is a one-pole filter creating
        // an analog-style exponential envelope curve.
        level = multiplier * (level - target) + target;

        // Done with the attack portion? Then go into decay. Notice that target
        // is 2.0 when the envelope is in the attack stage; that is how we tell
        // apart the different stages.
        if (level + target > 3.0f) {
            multiplier = decayMultiplier;
            target = sustainLevel;
        }

        return level;
    }

    inline bool isActive() const
    {
        return level > SILENCE;
    }

    inline bool isInAttack() const
    {
        return target >= 2.0f;
    }

    void attack()
    {
        // Make the envelope level greater than SILENCE, otherwise the voice
        // may not be seen as active.
        level += SILENCE + SILENCE;

        // Start the attack portion of the envelope. The target is not 1.0 but
        // 2.0 in order to make the attack steeper than a regular exponential
        // curve. The attack ends when the envelope level exceeds 1.0.
        target = 2.0f;
        multiplier = attackMultiplier;
    }

    void release()
    {
        target = 0.0f;
        multiplier = releaseMultiplier;
    }

    // Parameter values for this envelope.
    float attackMultiplier;
    float decayMultiplier;
    float sustainLevel;
    float releaseMultiplier;

    // Current envelope level.
    float level;

private:
    float target;
    float multiplier;
};

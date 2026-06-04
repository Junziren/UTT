#pragma once
#include <JuceHeader.h>

namespace UTT {

class PhaseProcessor
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setInvert(bool invertL, bool invertR);
    void setPhaseOffset(float degrees);
    void process(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate = 44100.0;
    bool invL = false, invR = false;
    float phaseOffset = 0.0f;  // degrees
    // 1st-order allpass state
    float apL = 0.0f, apR = 0.0f;
    float apCoeff = 0.0f;      // allpass coefficient
};

} // namespace UTT

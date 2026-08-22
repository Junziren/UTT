#pragma once
#include <JuceHeader.h>

namespace UTT {

class PhaseProcessor
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setInvert(bool invertL, bool invertR);
    void setPhaseOffset(float degrees); // 0 = bypass; otherwise target shift at referenceFrequency
    void process(juce::AudioBuffer<float>& buffer);

    // Frequency at which the requested phase offset is realised exactly.
    static constexpr double referenceFrequency = 1000.0;

private:
    double sampleRate = 44100.0;
    bool invL = false, invR = false;
    float phaseDegrees = 0.0f;
    float apCoeff = 0.0f; // shared by both cascaded sections
    // Two cascaded 1st-order allpass states per channel:
    // y[n] = c*x[n] + z;  z <- x[n] - c*y[n]
    float z1L = 0.0f, z1R = 0.0f;
    float z2L = 0.0f, z2R = 0.0f;
};

} // namespace UTT

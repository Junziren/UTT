#pragma once
#include <JuceHeader.h>

namespace UTT {

class Panner
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setPan(float val);             // -100 (full L) to +100 (full R), smoothed
    void setPanImmediate(float val);
    void process(juce::AudioBuffer<float>& buffer);

private:
    float panValue = 0.0f; // normalized -1..+1
    juce::SmoothedValue<float> gainLSm { 1.0f };
    juce::SmoothedValue<float> gainRSm { 1.0f };

    void applyPanTargets();
};

} // namespace UTT

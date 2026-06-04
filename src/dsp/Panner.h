#pragma once
#include <JuceHeader.h>

namespace UTT {

class Panner
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setPan(float val); // -100 (full L) to +100 (full R)
    void process(juce::AudioBuffer<float>& buffer);

private:
    float panValue = 0.0f; // normalized -1..+1
    float gainL = 1.0f, gainR = 1.0f;
};

} // namespace UTT

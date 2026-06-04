#pragma once
#include <JuceHeader.h>

namespace UTT {

class DCOffset
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate = 44100.0;
    float prevInL = 0.0f, prevInR = 0.0f;
    float prevOutL = 0.0f, prevOutR = 0.0f;
};

} // namespace UTT

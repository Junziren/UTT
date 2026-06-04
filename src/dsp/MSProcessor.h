#pragma once
#include <JuceHeader.h>

namespace UTT {

class MSProcessor
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setMode(int mode); // 0=Bypass, 1=Encode, 2=Decode
    void process(juce::AudioBuffer<float>& buffer);

private:
    int mode = 0; // 0=Bypass
};

} // namespace UTT

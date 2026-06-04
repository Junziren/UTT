#pragma once
#include <JuceHeader.h>

namespace UTT {

class PhaseMeter
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);
    float getCorrelation() const { return correlation.load(); }

private:
    std::atomic<float> correlation{0.0f};
    float smoothAlpha = 0.999f;
    float prevCorr = 0.0f;
};

} // namespace UTT

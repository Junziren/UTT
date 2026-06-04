#pragma once
#include <JuceHeader.h>

namespace UTT {

class Metering
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    float getLPeak() const { return lPeak.load(); }
    float getRPeak() const { return rPeak.load(); }
    float getLRMS() const { return lRMS.load(); }
    float getRRMS() const { return rRMS.load(); }

private:
    std::atomic<float> lPeak{-96.0f}, rPeak{-96.0f};
    std::atomic<float> lRMS{-96.0f}, rRMS{-96.0f};
    float rmsAlpha = 0.9995f; // smoothing
    float lRMSPrev = 0.0f, rRMSPrev = 0.0f;
};

} // namespace UTT

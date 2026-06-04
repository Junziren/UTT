#pragma once
#include <JuceHeader.h>

namespace UTT {

class LUFSMeter
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    float getMomentary() const  { return momentary.load(); }
    float getShortTerm() const  { return shortTerm.load(); }
    float getIntegrated() const { return integrated.load(); }

private:
    double sampleRate = 44100.0;
    std::atomic<float> momentary{-70.0f};
    std::atomic<float> shortTerm{-70.0f};
    std::atomic<float> integrated{-70.0f};

    // K-weighting filter state
    float k1State[2][2]{}; // [channel][state]
    float k2State[2][2]{};

    // Accumulators
    float momentaryAcc = 0.0f;
    float shortTermAcc = 0.0f;
    double integratedAcc = 0.0;
    int momentaryCount = 0;
    int shortTermCount = 0;
    juce::int64 integratedCount = 0;
};

} // namespace UTT

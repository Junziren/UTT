#pragma once
#include <JuceHeader.h>

namespace UTT {

class GainStage
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setGain(float db);
    void setGainImmediate(float db);
    void setOutputStage(float gainL, float gainR, bool muteL, bool muteR, bool soloL, bool soloR);
    void setOutputStageImmediate(float gainL, float gainR, bool muteL, bool muteR, bool soloL, bool soloR);
    void process(juce::AudioBuffer<float>& buffer);
    void processOutputStage(juce::AudioBuffer<float>& buffer);

private:
    juce::SmoothedValue<float> gainSm { 1.0f };
    juce::SmoothedValue<float> outGainLSm { 1.0f };
    juce::SmoothedValue<float> outGainRSm { 1.0f };
    bool muteL = false, muteR = false, soloL = false, soloR = false;
};

} // namespace UTT

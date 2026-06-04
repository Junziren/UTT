#pragma once
#include <JuceHeader.h>

namespace UTT {

class GainStage
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setGain(float db);
    void setOutputStage(float gainL, float gainR, bool muteL, bool muteR, bool soloL, bool soloR);
    void process(juce::AudioBuffer<float>& buffer);
    void processOutputStage(juce::AudioBuffer<float>& buffer);

private:
    float gainLinear = 1.0f;
    float outGainL = 1.0f, outGainR = 1.0f;
    bool muteL = false, muteR = false, soloL = false, soloR = false;
};

} // namespace UTT

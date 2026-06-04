#pragma once
#include <JuceHeader.h>

namespace UTT {

class ChannelProcessor
{
public:
    void prepare(double sr, int blockSize);
    void reset();

    void setMonoMode(int mode);        // 0=Off, 1=L+R Sum, 2=Mid Only, 3=Side Only
    void setSeparation(float percent); // 0-100
    void setMonoToStereo(int mode);    // 0=Off, 1=L->Both, 2=R->Both

    void process(juce::AudioBuffer<float>& buffer);
    void processPostSeparation(juce::AudioBuffer<float>& buffer);
    void swap(juce::AudioBuffer<float>& buffer);

private:
    int monoMode = 0;
    float separation = 1.0f; // 0.0-1.0 (0%=mono, 100%=stereo)
    int m2sMode = 0;
};

} // namespace UTT

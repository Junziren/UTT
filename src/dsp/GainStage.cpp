#include "GainStage.h"

namespace UTT {

void GainStage::prepare(double, int) {}
void GainStage::reset()
{
    gainLinear = 1.0f;
    outGainL = outGainR = 1.0f;
    muteL = muteR = soloL = soloR = false;
}

void GainStage::setGain(float db)
{
    gainLinear = std::pow(10.0f, db / 20.0f);
}

void GainStage::setOutputStage(float gainL, float gainR, bool ml, bool mr, bool sl, bool sr)
{
    outGainL = std::pow(10.0f, gainL / 20.0f);
    outGainR = std::pow(10.0f, gainR / 20.0f);
    muteL = ml;
    muteR = mr;
    soloL = sl;
    soloR = sr;
}

void GainStage::process(juce::AudioBuffer<float>& buffer)
{
    if (std::abs(gainLinear - 1.0f) < 0.0001f)
        return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        L[i] *= gainLinear;
        R[i] *= gainLinear;
    }
}

void GainStage::processOutputStage(juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    // Solo logic: priority over mute
    // Both solo -> normal output (no solo)
    // Only L solo -> mute R, output L
    // Only R solo -> mute L, output R
    bool effectiveMuteL = muteL;
    bool effectiveMuteR = muteR;

    if (soloL && !soloR)
    {
        effectiveMuteL = false;
        effectiveMuteR = true;
    }
    else if (!soloL && soloR)
    {
        effectiveMuteL = true;
        effectiveMuteR = false;
    }
    // Both solo or none: use mute as-is

    for (int i = 0; i < n; ++i)
    {
        L[i] *= outGainL;
        R[i] *= outGainR;
        if (effectiveMuteL) L[i] = 0.0f;
        if (effectiveMuteR) R[i] = 0.0f;
    }
}

} // namespace UTT

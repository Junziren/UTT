#include "GainStage.h"

namespace UTT {

void GainStage::prepare(double sr, int)
{
    gainSm.reset(sr, 0.01);
    outGainLSm.reset(sr, 0.01);
    outGainRSm.reset(sr, 0.01);
    gainSm.setCurrentAndTargetValue(1.0f);
    outGainLSm.setCurrentAndTargetValue(1.0f);
    outGainRSm.setCurrentAndTargetValue(1.0f);
}

void GainStage::reset()
{
    gainSm.setCurrentAndTargetValue(1.0f);
    outGainLSm.setCurrentAndTargetValue(1.0f);
    outGainRSm.setCurrentAndTargetValue(1.0f);
    muteL = muteR = soloL = soloR = false;
}

void GainStage::setGain(float db)
{
    gainSm.setTargetValue(std::pow(10.0f, db / 20.0f));
}

void GainStage::setGainImmediate(float db)
{
    gainSm.setCurrentAndTargetValue(std::pow(10.0f, db / 20.0f));
}

void GainStage::setOutputStage(float gainL, float gainR, bool ml, bool mr, bool sl, bool sr)
{
    outGainLSm.setTargetValue(std::pow(10.0f, gainL / 20.0f));
    outGainRSm.setTargetValue(std::pow(10.0f, gainR / 20.0f));
    muteL = ml;
    muteR = mr;
    soloL = sl;
    soloR = sr;
}

void GainStage::setOutputStageImmediate(float gainL, float gainR, bool ml, bool mr, bool sl, bool sr)
{
    outGainLSm.setCurrentAndTargetValue(std::pow(10.0f, gainL / 20.0f));
    outGainRSm.setCurrentAndTargetValue(std::pow(10.0f, gainR / 20.0f));
    muteL = ml;
    muteR = mr;
    soloL = sl;
    soloR = sr;
}

void GainStage::process(juce::AudioBuffer<float>& buffer)
{
    if (!gainSm.isSmoothing() && std::abs(gainSm.getCurrentValue() - 1.0f) < 1e-6f)
        return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    if (gainSm.isSmoothing())
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = gainSm.getNextValue();
            L[i] *= g;
            R[i] *= g;
        }
    }
    else
    {
        const float g = gainSm.getCurrentValue();
        for (int i = 0; i < n; ++i)
        {
            L[i] *= g;
            R[i] *= g;
        }
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

    const bool smoothing = outGainLSm.isSmoothing() || outGainRSm.isSmoothing();

    for (int i = 0; i < n; ++i)
    {
        const float gl = smoothing ? outGainLSm.getNextValue() : outGainLSm.getCurrentValue();
        const float gr = smoothing ? outGainRSm.getNextValue() : outGainRSm.getCurrentValue();

        L[i] = effectiveMuteL ? 0.0f : L[i] * gl;
        R[i] = effectiveMuteR ? 0.0f : R[i] * gr;
    }
}

} // namespace UTT

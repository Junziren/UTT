#include "Panner.h"

namespace UTT {

void Panner::prepare(double sr, int)
{
    gainLSm.reset(sr, 0.01);
    gainRSm.reset(sr, 0.01);
    gainLSm.setCurrentAndTargetValue(1.0f);
    gainRSm.setCurrentAndTargetValue(1.0f);
}

void Panner::reset()
{
    panValue = 0.0f;
    gainLSm.setCurrentAndTargetValue(1.0f);
    gainRSm.setCurrentAndTargetValue(1.0f);
}

void Panner::applyPanTargets()
{
    // Balance-style law with a cosine taper: the centre detent is unity
    // (transparent defaults), moving right attenuates only L, moving left
    // only R, converging continuously to a hard kill at the extremes.
    const float halfPi = juce::MathConstants<float>::halfPi;
    const float gl = panValue > 0.0f ? std::cos(panValue * halfPi) : 1.0f;
    const float gr = panValue < 0.0f ? std::cos(-panValue * halfPi) : 1.0f;
    gainLSm.setTargetValue(gl);
    gainRSm.setTargetValue(gr);
}

void Panner::setPan(float val)
{
    panValue = std::clamp(val / 100.0f, -1.0f, 1.0f);
    applyPanTargets();
}

void Panner::setPanImmediate(float val)
{
    panValue = std::clamp(val / 100.0f, -1.0f, 1.0f);
    const float halfPi = juce::MathConstants<float>::halfPi;
    const float gl = panValue > 0.0f ? std::cos(panValue * halfPi) : 1.0f;
    const float gr = panValue < 0.0f ? std::cos(-panValue * halfPi) : 1.0f;
    gainLSm.setCurrentAndTargetValue(gl);
    gainRSm.setCurrentAndTargetValue(gr);
}

void Panner::process(juce::AudioBuffer<float>& buffer)
{
    if (std::abs(panValue) < 0.001f
        && !gainLSm.isSmoothing() && !gainRSm.isSmoothing())
        return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    const bool smoothing = gainLSm.isSmoothing() || gainRSm.isSmoothing();

    for (int i = 0; i < n; ++i)
    {
        const float gl = smoothing ? gainLSm.getNextValue() : gainLSm.getCurrentValue();
        const float gr = smoothing ? gainRSm.getNextValue() : gainRSm.getCurrentValue();
        L[i] *= gl;
        R[i] *= gr;
    }
}

} // namespace UTT

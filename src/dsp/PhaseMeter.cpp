#include "PhaseMeter.h"

namespace UTT {

void PhaseMeter::prepare(double sr, int)
{
    // ~500 ms smoothing, sample-rate compensated
    smoothAlpha = std::exp(-1.0f / (static_cast<float>(sr) * 0.5f));
}

void PhaseMeter::reset()
{
    correlation = 0.0f;
    prevCorr = 0.0f;
}

void PhaseMeter::process(const juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getReadPointer(0);
    auto* R = buffer.getReadPointer(1);
    const int n = buffer.getNumSamples();

    float sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        sumLR += L[i] * R[i];
        sumL2 += L[i] * L[i];
        sumR2 += R[i] * R[i];
    }

    float denom = std::sqrt(sumL2 * sumR2);
    float instantCorr = (denom > 1e-10f) ? (sumLR / denom) : 0.0f;

    prevCorr = smoothAlpha * prevCorr + (1.0f - smoothAlpha) * instantCorr;
    correlation.store(prevCorr);
}

} // namespace UTT

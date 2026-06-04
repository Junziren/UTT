#include "Metering.h"

namespace UTT {

void Metering::prepare(double, int blockSize)
{
    // RMS smoothing: ~300ms time constant at 44.1kHz
    rmsAlpha = std::exp(-1.0f / (44100.0f * 0.3f));
    // Adjust alpha per sample rate later
}

void Metering::reset()
{
    lPeak = rPeak = -96.0f;
    lRMS = rRMS = -96.0f;
    lRMSPrev = rRMSPrev = 0.0f;
}

void Metering::process(const juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getReadPointer(0);
    auto* R = buffer.getReadPointer(1);
    const int n = buffer.getNumSamples();

    float peakL = 0.0f, peakR = 0.0f;
    float rmsL = lRMSPrev, rmsR = rRMSPrev;

    for (int i = 0; i < n; ++i)
    {
        float absL = std::abs(L[i]);
        float absR = std::abs(R[i]);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;
        rmsL = rmsAlpha * rmsL + (1.0f - rmsAlpha) * (L[i] * L[i]);
        rmsR = rmsAlpha * rmsR + (1.0f - rmsAlpha) * (R[i] * R[i]);
    }

    lRMSPrev = rmsL;
    rRMSPrev = rmsR;

    auto toDB = [](float v) -> float {
        if (v < 1e-10f) return -96.0f;
        return 20.0f * std::log10(v);
    };

    lPeak.store(toDB(peakL));
    rPeak.store(toDB(peakR));
    lRMS.store(toDB(std::sqrt(rmsL)));
    rRMS.store(toDB(std::sqrt(rmsR)));
}

} // namespace UTT

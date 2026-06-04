#include "DCOffset.h"

namespace UTT {

void DCOffset::prepare(double sr, int) { sampleRate = sr; reset(); }

void DCOffset::reset()
{
    prevInL = prevInR = 0.0f;
    prevOutL = prevOutR = 0.0f;
}

void DCOffset::process(juce::AudioBuffer<float>& buffer)
{
    // 1st-order DC blocking filter (high-pass at ~5Hz)
    // y[n] = x[n] - x[n-1] + alpha * y[n-1]
    const float fc = 5.0f;
    const float alpha = std::exp(-2.0f * juce::MathConstants<float>::pi * fc / static_cast<float>(sampleRate));

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float outL = L[i] - prevInL + alpha * prevOutL;
        float outR = R[i] - prevInR + alpha * prevOutR;

        prevInL = L[i];
        prevInR = R[i];
        prevOutL = outL;
        prevOutR = outR;

        L[i] = outL;
        R[i] = outR;
    }
}

} // namespace UTT

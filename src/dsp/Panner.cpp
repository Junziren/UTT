#include "Panner.h"

namespace UTT {

void Panner::prepare(double, int) {}
void Panner::reset() { panValue = 0.0f; gainL = gainR = 1.0f; }

void Panner::setPan(float val)
{
    panValue = std::clamp(val / 100.0f, -1.0f, 1.0f);

    // Sine/cosine pan law (-3dB center)
    float angle = (panValue + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    gainL = std::cos(angle);
    gainR = std::sin(angle);
}

void Panner::process(juce::AudioBuffer<float>& buffer)
{
    if (std::abs(panValue) < 0.001f) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        L[i] *= gainL;
        R[i] *= gainR;
    }
}

} // namespace UTT

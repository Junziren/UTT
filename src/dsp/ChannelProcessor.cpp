#include "ChannelProcessor.h"

namespace UTT {

void ChannelProcessor::prepare(double sr, int)
{
    separationSm.reset(sr, 0.01);
    separationSm.setCurrentAndTargetValue(separation);
}

void ChannelProcessor::reset()
{
    separationSm.setCurrentAndTargetValue(1.0f);
}

void ChannelProcessor::setMonoMode(int mode) { monoMode = mode; }
void ChannelProcessor::setSeparation(float percent)
{
    separation = juce::jlimit(0.0f, 1.0f, percent / 100.0f);
    separationSm.setTargetValue(separation);
}
void ChannelProcessor::setSeparationImmediate(float percent)
{
    separation = juce::jlimit(0.0f, 1.0f, percent / 100.0f);
    separationSm.setCurrentAndTargetValue(separation);
}
void ChannelProcessor::setMonoToStereo(int mode) { m2sMode = mode; }

void ChannelProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (monoMode == 0)
        return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    switch (monoMode)
    {
    case 1: // L+R Sum -> both channels
        for (int i = 0; i < n; ++i)
        {
            float sum = (L[i] + R[i]) * 0.5f;
            L[i] = R[i] = sum;
        }
        break;

    case 2: // Mid Only: L = R = (L+R)/2
        for (int i = 0; i < n; ++i)
        {
            float mid = (L[i] + R[i]) * 0.5f;
            L[i] = R[i] = mid;
        }
        break;

    case 3: // Side Only: L = R = (L-R)/2
        for (int i = 0; i < n; ++i)
        {
            float side = (L[i] - R[i]) * 0.5f;
            L[i] = R[i] = side;
        }
        break;

    default:
        break;
    }
}

void ChannelProcessor::processPostSeparation(juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    // Stereo separation: M/S rebalance, side scaled by the separation amount.
    // After any mono mode L == R, so the side signal is already zero and this
    // is a mathematical no-op there; it narrows true stereo material only.
    if (separation < 0.9995f || separationSm.isSmoothing())
    {
        for (int i = 0; i < n; ++i)
        {
            const float sep = separationSm.getNextValue();
            float mid = (L[i] + R[i]) * 0.5f;
            float side = (L[i] - R[i]) * 0.5f * sep;
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

    // Mono to Stereo copy
    if (m2sMode == 0)
        return;

    if (m2sMode == 1) // L -> Both
    {
        for (int i = 0; i < n; ++i)
            R[i] = L[i];
    }
    else if (m2sMode == 2) // R -> Both
    {
        for (int i = 0; i < n; ++i)
            L[i] = R[i];
    }
}

void ChannelProcessor::swap(juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
        std::swap(L[i], R[i]);
}

} // namespace UTT

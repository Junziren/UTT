#include "ChannelProcessor.h"

namespace UTT {

void ChannelProcessor::prepare(double, int) {}
void ChannelProcessor::reset()
{
    monoMode = 0;
    separation = 1.0f;
    m2sMode = 0;
}

void ChannelProcessor::setMonoMode(int mode) { monoMode = mode; }
void ChannelProcessor::setSeparation(float percent) { separation = percent / 100.0f; }
void ChannelProcessor::setMonoToStereo(int mode) { m2sMode = mode; }

void ChannelProcessor::process(juce::AudioBuffer<float>& buffer)
{
    // Mono mode: modify L/R contents directly
    if (monoMode == 0) return;

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
    }

    // Apply stereo separation (adjust mid/side balance)
    if (separation < 0.999f && monoMode == 0)
    {
        for (int i = 0; i < n; ++i)
        {
            float mid = (L[i] + R[i]) * 0.5f;
            float side = (L[i] - R[i]) * 0.5f * separation;
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }
}

void ChannelProcessor::processPostSeparation(juce::AudioBuffer<float>& buffer)
{
    // Apply stereo separation even when monoMode was not 0
    if (separation < 0.999f && monoMode != 0)
    {
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);
        const int n = buffer.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            float mid = (L[i] + R[i]) * 0.5f;
            float side = (L[i] - R[i]) * 0.5f * separation;
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

    // Mono to Stereo copy
    if (m2sMode == 0) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

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

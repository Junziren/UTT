#include "Oscilloscope.h"

namespace UTT {

void Oscilloscope::prepare(double sr, int)
{
    sampleRate = sr;
    points.resize(maxPoints, {0.0f, 0.0f});
}

void Oscilloscope::reset()
{
    std::fill(points.begin(), points.end(), Point{0.0f, 0.0f});
    writeIndex = 0;
}

void Oscilloscope::process(const juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getReadPointer(0);
    auto* R = buffer.getReadPointer(1);
    const int n = buffer.getNumSamples();

    int step = std::max(1, n / maxPoints);
    for (int i = 0; i < n; i += step)
    {
        if (writeIndex >= maxPoints)
            writeIndex = 0;
        points[writeIndex] = { L[i], R[i] };
        ++writeIndex;
    }
}

} // namespace UTT

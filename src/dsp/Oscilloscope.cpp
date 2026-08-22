#include "Oscilloscope.h"

namespace UTT {

void Oscilloscope::prepare(double sr, int)
{
    sampleRate = sr;
    // 50 ms capture window
    windowSamples = juce::jlimit(maxPoints, 1 << 20, static_cast<int>(0.05 * sr));
    ringL.assign(windowSamples, 0.0f);
    ringR.assign(windowSamples, 0.0f);
    for (auto& f : frames)
    {
        f.l.assign(maxPoints, 0.0f);
        f.r.assign(maxPoints, 0.0f);
    }
    published.store(0);
    generation.store(0);
    writePos = 0;
}

void Oscilloscope::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos = 0;
}

void Oscilloscope::process(const juce::AudioBuffer<float>& buffer)
{
    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getReadPointer(1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        ringL[writePos] = L[i];
        ringR[writePos] = R[i];
        if (++writePos >= windowSamples)
            writePos = 0;
    }

    // Publish a decimated, oldest-to-newest copy of the most recent window.
    const int pub = published.load(std::memory_order_relaxed);
    const int w = (pub + 1) % 3;
    Frame& out = frames[static_cast<size_t>(w)];

    const int newest = (writePos + windowSamples - 1) % windowSamples;
    const float stride = static_cast<float>(windowSamples - 1) / static_cast<float>(maxPoints - 1);
    for (int k = 0; k < maxPoints; ++k)
    {
        int idx = newest - static_cast<int>(k * stride + 0.5f);
        if (idx < 0)
            idx += windowSamples;
        out.l[static_cast<size_t>(k)] = ringL[static_cast<size_t>(idx)];
        out.r[static_cast<size_t>(k)] = ringR[static_cast<size_t>(idx)];
    }

    published.store(w, std::memory_order_release);
    generation.fetch_add(1, std::memory_order_release);
}

std::uint64_t Oscilloscope::copyLatestFrame(std::vector<float>& l, std::vector<float>& r) const
{
    const std::uint64_t g0 = generation.load(std::memory_order_acquire);
    const int p = published.load(std::memory_order_relaxed);
    const Frame& f = frames[static_cast<size_t>(p)];
    l.assign(f.l.begin(), f.l.end());
    r.assign(f.r.begin(), f.r.end());
    const std::uint64_t g1 = generation.load(std::memory_order_acquire);
    return g0 == g1 ? g1 : 0;
}

} // namespace UTT

#include "MSProcessor.h"

namespace UTT {

void MSProcessor::prepare(double, int) {}
void MSProcessor::reset() { mode = 0; }
void MSProcessor::setMode(int m) { mode = m; }

void MSProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (mode == 0) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    const float half = 0.5f;

    if (mode == 1) // Encode: L/R -> M/S
    {
        for (int i = 0; i < n; ++i)
        {
            float mid = (L[i] + R[i]) * half;
            float side = (L[i] - R[i]) * half;
            L[i] = mid;
            R[i] = side;
        }
    }
    else if (mode == 2) // Decode: M/S -> L/R
    {
        for (int i = 0; i < n; ++i)
        {
            float left = L[i] + R[i];
            float right = L[i] - R[i];
            L[i] = left;
            R[i] = right;
        }
    }
}

} // namespace UTT

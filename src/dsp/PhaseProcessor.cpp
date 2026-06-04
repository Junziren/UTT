#include "PhaseProcessor.h"

namespace UTT {

void PhaseProcessor::prepare(double sr, int) { sampleRate = sr; }

void PhaseProcessor::reset()
{
    apL = apR = 0.0f;
    apCoeff = 0.0f;
}

void PhaseProcessor::setInvert(bool invertL, bool invertR)
{
    invL = invertL;
    invR = invertR;
}

void PhaseProcessor::setPhaseOffset(float degrees)
{
    // Map 0~360 degrees to allpass coefficient for fixed 1kHz center
    // For a 1st-order allpass: phi = -2*atan(omega/omega0) + pi
    // We approximate with a simple coefficient based on degrees
    float norm = degrees / 360.0f;
    // Use a simple allpass at 1kHz: map 0-360 to coefficient -0.99..0.99
    apCoeff = -0.99f + 1.98f * norm;
}

void PhaseProcessor::process(juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    if (invL || invR || std::abs(apCoeff) > 0.001f)
    {
        for (int i = 0; i < n; ++i)
        {
            // Phase invert
            if (invL) L[i] = -L[i];
            if (invR) R[i] = -R[i];

            // 1st-order allpass filter for phase offset
            if (std::abs(apCoeff) > 0.001f)
            {
                float xL = L[i], xR = R[i];
                L[i] = apCoeff * xL + apL;
                R[i] = apCoeff * xR + apR;
                apL = xL - apCoeff * L[i];
                apR = xR - apCoeff * R[i];
            }
        }
    }
}

} // namespace UTT

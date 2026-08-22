#include "PhaseProcessor.h"

namespace UTT {

void PhaseProcessor::prepare(double sr, int) { sampleRate = sr; }

void PhaseProcessor::reset()
{
    z1L = z1R = 0.0f;
    z2L = z2R = 0.0f;
}

void PhaseProcessor::setInvert(bool invertL, bool invertR)
{
    invL = invertL;
    invR = invertR;
}

void PhaseProcessor::setPhaseOffset(float degrees)
{
    phaseDegrees = juce::jlimit(0.0f, 360.0f, degrees);

    // Two cascaded 1st-order allpass sections, each contributing half the
    // requested shift at referenceFrequency. For H(z) = (c + z^-1)/(1 + c z^-1)
    // the phase at f is phi = -2 atan( tan(pi f / fs) * (1-c)/(1+c) ), so the
    // coefficient realising a per-section shift of theta at f0 is
    //   c = (1 - r) / (1 + r),   r = tan(theta / 2) / tan(pi f0 / fs)
    // The pole approaches -1 as the request nears 360 degrees; r is clamped
    // so it stays inside the unit circle. A first-order pair cannot hold the
    // shift constant across the spectrum - the knob is exact at 1 kHz only.
    const float theta = juce::degreesToRadians(phaseDegrees) * 0.5f;
    const float t0 = std::tan(juce::MathConstants<float>::pi
                              * static_cast<float>(referenceFrequency)
                              / static_cast<float>(sampleRate));
    float r = std::tan(theta * 0.5f) / t0;
    r = std::min(r, 1000.0f);
    apCoeff = (1.0f - r) / (1.0f + r);
    if (!(apCoeff > -1.0f && apCoeff < 1.0f)) // NaN / degenerate sample rate guard
    {
        apCoeff = 0.0f;
        phaseDegrees = 0.0f;
    }
}

void PhaseProcessor::process(juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    const bool runAllpass = phaseDegrees >= 0.05f;
    const float c = apCoeff;

    if (!invL && !invR && !runAllpass)
        return;

    for (int i = 0; i < n; ++i)
    {
        float xl = L[i], xr = R[i];

        if (invL) xl = -xl;
        if (invR) xr = -xr;

        if (runAllpass)
        {
            float yl = c * xl + z1L;  z1L = xl - c * yl;
            float yr = c * xr + z1R;  z1R = xr - c * yr;

            float y2l = c * yl + z2L; z2L = yl - c * y2l;
            float y2r = c * yr + z2R; z2R = yr - c * y2r;

            xl = y2l;
            xr = y2r;
        }

        L[i] = xl;
        R[i] = xr;
    }
}

} // namespace UTT

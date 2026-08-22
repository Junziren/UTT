#include "LUFSMeter.h"
#include <cmath>

namespace UTT {

LUFSMeter::KWeightCoeffs LUFSMeter::computeStage1(double sampleRate)
{
    // BS.1770 pre-filter (high shelf), discretised with the ITU bilinear
    // design that reproduces the standard's 48 kHz coefficient table exactly
    // and generalises to any sample rate.
    constexpr double G  = 3.999843853973347;
    constexpr double fc = 1681.9744509555319;
    constexpr double Q  = 0.7071752369554196;

    const double K  = std::tan(juce::MathConstants<double>::pi * fc / sampleRate);
    const double Vh = std::pow(10.0, G / 20.0);
    const double Vb = std::pow(Vh, 0.4996667741545416);

    const double a0 = 1.0 + K / Q + K * K;

    KWeightCoeffs c;
    c.b0 = (Vh + Vb * K / Q + K * K) / a0;
    c.b1 = 2.0 * (K * K - Vh) / a0;
    c.b2 = (Vh - Vb * K / Q + K * K) / a0;
    c.a1 = 2.0 * (K * K - 1.0) / a0;
    c.a2 = (1.0 - K / Q + K * K) / a0;
    return c;
}

LUFSMeter::KWeightCoeffs LUFSMeter::computeStage2(double sampleRate)
{
    // BS.1770 RLB weighting (high-pass), same ITU bilinear design.
    constexpr double fc = 38.13547087602444;
    constexpr double Q  = 0.5003270373238773;

    const double K  = std::tan(juce::MathConstants<double>::pi * fc / sampleRate);
    const double a0 = 1.0 + K / Q + K * K;

    KWeightCoeffs c;
    c.b0 = 1.0;
    c.b1 = -2.0;
    c.b2 = 1.0;
    c.a1 = 2.0 * (K * K - 1.0) / a0;
    c.a2 = (1.0 - K / Q + K * K) / a0;
    return c;
}

void LUFSMeter::prepare(double sr, int)
{
    sampleRate = sr;
    k1 = computeStage1(sr);
    k2 = computeStage2(sr);
    gateBlocks.assign(maxGateBlocks, 0.0);
    reset();
}

void LUFSMeter::reset()
{
    momentary.store(-70.0f);
    shortTerm.store(-70.0f);
    integrated.store(-70.0f);

    std::memset(k1State, 0, sizeof(k1State));
    std::memset(k2State, 0, sizeof(k2State));
    std::memset(chunkSums, 0, sizeof(chunkSums));
    chunkSamples = 0;
    chunkCount = 0;
    chunkWrite = 0;
    gateBlockCount = 0;
    blocksSinceIntegratedUpdate = 0;
}

static float loudnessFromPower(double sumMeanSquare)
{
    if (sumMeanSquare <= 0.0)
        return -70.0f;
    return static_cast<float>(juce::jlimit(-70.0, 0.0,
                        -0.691 + 10.0 * std::log10(sumMeanSquare)));
}

// Sum mean square over the most recent `count` stored chunks, all channels.
void LUFSMeter::publishMomentary()
{
    const int count = juce::jmin(chunkCount, 4); // 4 x 100 ms
    if (count == 0) return;

    double sum = 0.0;
    for (int k = 0; k < count; ++k)
    {
        const int idx = (chunkWrite - 1 - k + chunkHistory) % chunkHistory;
        for (int ch = 0; ch < numChannels; ++ch)
            sum += chunkSums[idx][ch];
    }
    const int totalSamples = count * static_cast<int>(0.1 * sampleRate);
    momentary.store(loudnessFromPower(sum / totalSamples));
}

void LUFSMeter::publishShortTerm()
{
    const int count = juce::jmin(chunkCount, chunkHistory); // 30 x 100 ms
    if (count == 0) return;

    double sum = 0.0;
    for (int k = 0; k < count; ++k)
    {
        const int idx = (chunkWrite - 1 - k + chunkHistory) % chunkHistory;
        for (int ch = 0; ch < numChannels; ++ch)
            sum += chunkSums[idx][ch];
    }
    const int totalSamples = count * static_cast<int>(0.1 * sampleRate);
    shortTerm.store(loudnessFromPower(sum / totalSamples));
}

void LUFSMeter::recomputeIntegrated()
{
    if (gateBlockCount == 0)
    {
        integrated.store(-70.0f);
        return;
    }

    // Relative threshold: average over blocks that passed the absolute gate.
    double sumZ = 0.0;
    for (int i = 0; i < gateBlockCount; ++i)
        sumZ += gateBlocks[i];
    const double gamma = -0.691 + 10.0 * std::log10(sumZ / gateBlockCount);

    double sumG = 0.0;
    int nG = 0;
    for (int i = 0; i < gateBlockCount; ++i)
    {
        // Inclusive comparison: a perfectly constant signal must not gate
        // itself out (every block sits exactly at the threshold).
        if (-0.691 + 10.0 * std::log10(gateBlocks[i]) >= gamma)
        {
            sumG += gateBlocks[i];
            ++nG;
        }
    }

    if (nG == 0)
    {
        integrated.store(-70.0f);
        return;
    }
    integrated.store(loudnessFromPower(sumG / nG));
}

void LUFSMeter::process(const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const float* in[numChannels] = { buffer.getReadPointer(0),
                                     buffer.getReadPointer(1) };

    const int chunkSize = static_cast<int>(0.1 * sampleRate);

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float x = in[ch][i];

            const float y1 = static_cast<float>(k1.b0) * x + k1State[ch][0];
            k1State[ch][0] = static_cast<float>(k1.b1) * x - static_cast<float>(k1.a1) * y1 + k1State[ch][1];
            k1State[ch][1] = static_cast<float>(k1.b2) * x - static_cast<float>(k1.a2) * y1;

            const float y2 = static_cast<float>(k2.b0) * y1 + k2State[ch][0];
            k2State[ch][0] = static_cast<float>(k2.b1) * y1 - static_cast<float>(k2.a1) * y2 + k2State[ch][1];
            k2State[ch][1] = static_cast<float>(k2.b2) * y1 - static_cast<float>(k2.a2) * y2;

            chunkSums[chunkWrite][ch] += static_cast<double>(y2) * static_cast<double>(y2);
        }

        if (++chunkSamples >= chunkSize)
        {
            ++chunkCount;
            chunkWrite = (chunkWrite + 1) % chunkHistory;
            for (int ch = 0; ch < numChannels; ++ch)
                chunkSums[chunkWrite][ch] = 0.0;
            chunkSamples = 0;

            publishMomentary();
            publishShortTerm();

            // Gating block: 400 ms window ending at this chunk boundary.
            if (chunkCount >= 4 && gateBlockCount < maxGateBlocks)
            {
                double sum = 0.0;
                for (int k = 0; k < 4; ++k)
                {
                    const int idx = (chunkWrite - 1 - k + chunkHistory) % chunkHistory;
                    for (int ch = 0; ch < numChannels; ++ch)
                        sum += chunkSums[idx][ch];
                }
                const double z = sum / (4.0 * chunkSize);
                if (loudnessFromPower(z) > absoluteGateLUFS)
                    gateBlocks[gateBlockCount++] = z;
            }

            if (++blocksSinceIntegratedUpdate >= 5) // refresh ~every 500 ms
            {
                blocksSinceIntegratedUpdate = 0;
                recomputeIntegrated();
            }
        }
    }
}

} // namespace UTT

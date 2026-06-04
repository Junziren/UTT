#include "LUFSMeter.h"

namespace UTT {

void LUFSMeter::prepare(double sr, int)
{
    sampleRate = sr;
    reset();
}

void LUFSMeter::reset()
{
    momentary = shortTerm = integrated = -70.0f;
    momentaryAcc = shortTermAcc = 0.0f;
    integratedAcc = 0.0;
    momentaryCount = shortTermCount = 0;
    integratedCount = 0;
    std::memset(k1State, 0, sizeof(k1State));
    std::memset(k2State, 0, sizeof(k2State));
}

void LUFSMeter::process(const juce::AudioBuffer<float>& buffer)
{
    auto* L = buffer.getReadPointer(0);
    auto* R = buffer.getReadPointer(1);
    const int n = buffer.getNumSamples();

    // K-weighting filter coefficients (simplified for 48kHz, will adjust)
    // Stage 1: high-pass, 38.5 Hz
    // Stage 2: high-shelf, 1500 Hz +4dB
    const float b1_0 = 1.53512485958697f;
    const float b1_1 = -2.69169618940638f;
    const float b1_2 = 1.19839281085285f;
    const float a1_1 = -1.69065929318241f;
    const float a1_2 = 0.73248077421585f;

    const float b2_0 = 1.0f;
    const float b2_1 = -2.0f;
    const float b2_2 = 1.0f;
    const float a2_1 = -1.99004745483398f;
    const float a2_2 = 0.99007225036621f;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < n; ++i)
        {
            float x = data[i];

            // Stage 1
            float y1 = b1_0 * x + k1State[ch][0];
            k1State[ch][0] = b1_1 * x - a1_1 * y1 + k1State[ch][1];
            k1State[ch][1] = b1_2 * x - a1_2 * y1;

            // Stage 2
            float y2 = b2_0 * y1 + k2State[ch][0];
            k2State[ch][0] = b2_1 * y1 - a2_1 * y2 + k2State[ch][1];
            k2State[ch][1] = b2_2 * y1 - a2_2 * y2;

            float power = y2 * y2;
            momentaryAcc += power;
            shortTermAcc += power;
            integratedAcc += static_cast<double>(power);
        }
    }

    int newSamples = n * 2;
    momentaryCount += newSamples;
    shortTermCount += newSamples;
    integratedCount += newSamples;

    // Momentary: 400ms window
    int momWin = static_cast<int>(0.4 * sampleRate);
    if (momentaryCount >= momWin)
    {
        float avg = momentaryAcc / static_cast<float>(momentaryCount);
        float lufs = -0.691f + 10.0f * std::log10(std::max(avg, 1e-10f));
        momentary.store(std::clamp(lufs, -70.0f, 0.0f));
        momentaryAcc = 0.0f;
        momentaryCount = 0;
    }

    // Short-term: 3s window
    int stWin = static_cast<int>(3.0 * sampleRate);
    if (shortTermCount >= stWin)
    {
        float avg = shortTermAcc / static_cast<float>(shortTermCount);
        float lufs = -0.691f + 10.0f * std::log10(std::max(avg, 1e-10f));
        shortTerm.store(std::clamp(lufs, -70.0f, 0.0f));
        shortTermAcc = 0.0f;
        shortTermCount = 0;
    }

    // Integrated: running average
    if (integratedCount > 0)
    {
        double avg = integratedAcc / static_cast<double>(integratedCount);
        float lufs = -0.691f + 10.0f * std::log10(std::max(avg, 1e-10));
        integrated.store(std::clamp(lufs, -70.0f, 0.0f));
    }
}

} // namespace UTT

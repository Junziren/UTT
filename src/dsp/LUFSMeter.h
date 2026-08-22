#pragma once
#include <JuceHeader.h>
#include <atomic>

namespace UTT {

// ITU-R BS.1770-4 / EBU R128 loudness meter.
// K-weighting is designed for the actual sample rate; momentary/short-term
// are 400 ms / 3 s sliding windows and integrated loudness uses the standard
// 400 ms blocks stepped every 100 ms with absolute (-70 LUFS) and relative
// (-10 LU) gating.
class LUFSMeter
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    float getMomentary() const  { return momentary.load(std::memory_order_relaxed); }
    float getShortTerm() const  { return shortTerm.load(std::memory_order_relaxed); }
    float getIntegrated() const { return integrated.load(std::memory_order_relaxed); }

    struct KWeightCoeffs
    {
        // a0-normalised biquad coefficients
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    // Stage 1: high-shelf (+4 dB @ ~1682 Hz), stage 2: high-pass (~38 Hz).
    static KWeightCoeffs computeStage1(double sampleRate);
    static KWeightCoeffs computeStage2(double sampleRate);

    static constexpr int numChannels = 2;

private:
    void publishMomentary();
    void publishShortTerm();
    void recomputeIntegrated();

    double sampleRate = 48000.0;
    std::atomic<float> momentary {-70.0f};
    std::atomic<float> shortTerm {-70.0f};
    std::atomic<float> integrated{-70.0f};

    // K-weighting biquads (transposed direct form II is not used; DF1-style
    // two-state transposed form per stage, per channel)
    KWeightCoeffs k1, k2;
    float k1State[numChannels][2] {};
    float k2State[numChannels][2] {};

    // 100 ms power chunks (sum of squares per channel)
    static constexpr int chunkHistory = 30; // 3 s of 100 ms chunks
    double chunkSums[chunkHistory][numChannels] {};
    int chunkSamples = 0;        // samples accumulated in the open chunk
    int chunkCount = 0;          // chunks stored in the ring (up to chunkHistory)
    int chunkWrite = 0;          // ring write position

    // Gating block history for integrated loudness. Capacity covers 6 hours;
    // when full, new blocks stop contributing (displayed value freezes).
    static constexpr int maxGateBlocks = 216000;
    std::vector<double> gateBlocks; // z = sum over channels of window mean square
    int gateBlockCount = 0;

    static constexpr double absoluteGateLUFS = -70.0;
    int blocksSinceIntegratedUpdate = 0;
};

} // namespace UTT

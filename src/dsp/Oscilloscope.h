#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace UTT {

// Stereo waveform capture with a lock-free triple buffer:
// the audio thread publishes decimated, time-ordered frames; readers copy
// the newest complete frame and discard it if a newer one landed mid-copy.
class Oscilloscope
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    // Reader thread (message/bridge only). Copies the newest complete frame
    // into the caller's vectors (pre-sized to maxPoints). Returns the frame
    // generation, or 0 if the frame was torn by a concurrent publish.
    std::uint64_t copyLatestFrame(std::vector<float>& l, std::vector<float>& r) const;

    static constexpr int maxPoints = 2048;

private:
    double sampleRate = 44100.0;
    int windowSamples = maxPoints;
    std::vector<float> ringL, ringR;
    int writePos = 0;

    struct Frame { std::vector<float> l, r; };
    std::array<Frame, 3> frames;
    std::atomic<int> published { 0 };
    std::atomic<std::uint64_t> generation { 0 };
};

} // namespace UTT

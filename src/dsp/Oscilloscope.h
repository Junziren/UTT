#pragma once
#include <JuceHeader.h>

namespace UTT {

class Oscilloscope
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    struct Point { float x, y; };
    const std::vector<Point>& getPoints() const { return points; }
    int getNumPoints() const { return static_cast<int>(points.size()); }

private:
    static constexpr int maxPoints = 2048;
    std::vector<Point> points;
    int writeIndex = 0;
    double sampleRate = 44100.0;
};

} // namespace UTT

#pragma once
#include <JuceHeader.h>

namespace UTT {

class AudioRecorder
{
public:
    void prepare(double sr, int blockSize);
    void reset();
    void setPath(const juce::File& f);
    void startRecording();
    void stopRecording();
    void process(const juce::AudioBuffer<float>& buffer);
    bool isRecording() const { return recording; }

private:
    double sampleRate = 44100.0;
    juce::File outputFile;
    bool recording = false;
    std::unique_ptr<juce::FileOutputStream> fileStream;
    std::unique_ptr<juce::WavAudioFormat> wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
};

} // namespace UTT

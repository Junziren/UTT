#include "AudioRecorder.h"

namespace UTT {

void AudioRecorder::prepare(double sr, int)
{
    sampleRate = sr;
}

void AudioRecorder::reset()
{
    stopRecording();
}

void AudioRecorder::setPath(const juce::File& f)
{
    if (f != outputFile)
    {
        stopRecording();
        outputFile = f;
    }
}

void AudioRecorder::startRecording()
{
    if (recording || outputFile.getFullPathName().isEmpty())
        return;

    outputFile.getParentDirectory().createDirectory();
    outputFile.deleteFile();

    fileStream = std::make_unique<juce::FileOutputStream>(outputFile);
    if (fileStream->failedToOpen())
    {
        fileStream.reset();
        return;
    }

    wavFormat = std::make_unique<juce::WavAudioFormat>();
    writer.reset(wavFormat->createWriterFor(
        fileStream.get(),
        sampleRate,
        2,
        24,
        {},
        0));

    if (writer)
        recording = true;
    else
        fileStream.reset();  // Writer creation failed, close the file
}

void AudioRecorder::stopRecording()
{
    recording = false;
    writer.reset();
    wavFormat.reset();
    fileStream.reset();
}

void AudioRecorder::process(const juce::AudioBuffer<float>& buffer)
{
    if (!recording || !writer)
        return;

    const float* const* channels = buffer.getArrayOfReadPointers();
    writer->writeFromFloatArrays(channels, 2, buffer.getNumSamples());
}

} // namespace UTT

#include "AudioRecorder.h"
#include <cstring>

namespace UTT {

AudioRecorder::~AudioRecorder()
{
    desiredOn.store(false, std::memory_order_relaxed);
    if (writerThread != nullptr)
        writerThread->stopThread(3000);
    // The thread's run() loop drains the FIFO and finalises the file on exit.
}

void AudioRecorder::allocateFifo(int slots)
{
    slots = juce::nextPowerOfTwo(juce::jmax(slots, 8));
    fifo.clear();
    fifo.resize(slots);
    for (auto& s : fifo)
    {
        s.l.assign(static_cast<size_t>(slotSamples), 0.0f);
        s.r.assign(static_cast<size_t>(slotSamples), 0.0f);
        s.count = 0;
    }
    fifoMask = static_cast<std::uint32_t>(slots - 1);
    readPos.store(0);
    writePos.store(0);
}

void AudioRecorder::prepare(double sr, int maxBlock)
{
    sampleRate = sr;
    openSampleRate = sr;
    slotSamples = juce::jmax(32, maxBlock);

    const int wantedSlots = static_cast<int>((2.0 * sr) / slotSamples) + 1;
    const bool sizeChanged = fifo.empty()
        || static_cast<int>(fifo[0].l.size()) != slotSamples
        || static_cast<int>(fifo.size()) != juce::nextPowerOfTwo(juce::jmax(wantedSlots, 8));
    if (sizeChanged)
        allocateFifo(wantedSlots);

    if (writerThread == nullptr)
    {
        writerThread = std::make_unique<WriterThread>(*this);
        writerThread->startThread();
    }
}

void AudioRecorder::reset()
{
    desiredOn.store(false, std::memory_order_relaxed);
}

void AudioRecorder::setPath(const juce::String& path)
{
    {
        const juce::ScopedLock lock(pathLock);
        pathString = path;
        pathIsAutomatic = false;
    }
    pathVersion.fetch_add(1, std::memory_order_release); // writer rolls the take over
}

juce::String AudioRecorder::getPath() const
{
    const juce::ScopedLock lock(pathLock);
    return pathString;
}

void AudioRecorder::setRecordEnabled(bool enabled)
{
    desiredOn.store(enabled, std::memory_order_relaxed);
}

void AudioRecorder::process(const float* const* channels, int numSamples)
{
    if (!active.load(std::memory_order_acquire) || fifo.empty())
        return;

    int offset = 0;
    while (offset < numSamples)
    {
        const auto w = writePos.load(std::memory_order_relaxed);
        const auto r = readPos.load(std::memory_order_acquire);
        if (w - r >= fifo.size()) // ring full: disk stalled, drop the rest
        {
            droppedBlocks.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        Slot& s = fifo[w & fifoMask];
        const int chunk = juce::jmin(numSamples - offset, slotSamples);
        std::memcpy(s.l.data(), channels[0] + offset, static_cast<size_t>(chunk) * sizeof(float));
        std::memcpy(s.r.data(), channels[1] + offset, static_cast<size_t>(chunk) * sizeof(float));
        s.count = chunk;
        writePos.store(w + 1, std::memory_order_release);
        offset += chunk;
    }
}

bool AudioRecorder::consumeError()
{
    return errorFlag.exchange(false, std::memory_order_acq_rel);
}

bool AudioRecorder::openFile()
{
    juce::String path;
    bool needsAutomaticPath = false;

    {
        const juce::ScopedLock lock(pathLock);
        needsAutomaticPath = pathString.isEmpty() || pathIsAutomatic;
        if (!needsAutomaticPath)
            path = pathString;
    }

    if (needsAutomaticPath)
    {
        const auto automaticPath = getDefaultPath().getFullPathName();
        const juce::ScopedLock lock(pathLock);

        // A simultaneous BROWSE action wins over the automatically generated
        // file. This lock is never acquired from the audio callback.
        if (pathString.isEmpty() || pathIsAutomatic)
        {
            pathString = automaticPath;
            pathIsAutomatic = true;
        }

        path = pathString;
    }

    const juce::File file(path);
    if (!file.getParentDirectory().createDirectory().wasOk())
        return false;
    file.deleteFile();

    auto stream = std::make_unique<juce::FileOutputStream>(file);
    if (stream->failedToOpen())
        return false;

    wavFormat = std::make_unique<juce::WavAudioFormat>();
    writer.reset(wavFormat->createWriterFor(stream.get(), openSampleRate, 2, 24, {}, 0));
    if (writer == nullptr)
    {
        wavFormat.reset();
        return false; // stream still owned by the unique_ptr, freed here
    }

    stream.release(); // the writer now owns the stream and deletes it
    return true;
}

void AudioRecorder::closeFile()
{
    writer.reset();     // flushes, patches the WAV header and deletes the stream
    wavFormat.reset();
}

bool AudioRecorder::drainFifo()
{
    bool wrote = false;
    for (;;)
    {
        const auto r = readPos.load(std::memory_order_relaxed);
        const auto w = writePos.load(std::memory_order_acquire);
        if (r == w)
            break;

        Slot& s = fifo[r & fifoMask];
        const float* chans[2] = { s.l.data(), s.r.data() };
        if (writer == nullptr
            || !writer->writeFromFloatArrays(chans, 2, s.count))
        {
            errorFlag.store(true, std::memory_order_release);
            closeFile();
            active.store(false, std::memory_order_release);
            // Drop the backlog so a later take doesn't inherit stale audio.
            readPos.store(writePos.load(std::memory_order_relaxed),
                          std::memory_order_release);
            return true;
        }
        readPos.store(r + 1, std::memory_order_release);
        wrote = true;
    }
    return wrote;
}

void AudioRecorder::writerLoop(juce::Thread& thread)
{
    while (!thread.threadShouldExit())
    {
        bool worked = false;

        if (active.load(std::memory_order_relaxed))
        {
            worked = drainFifo();

            if (pathVersion.load(std::memory_order_acquire) != openedPathVersion)
            {
                // New path chosen mid-take: finalise the old file, start a new one.
                closeFile();
                if (openFile())
                {
                    openedPathVersion = pathVersion.load(std::memory_order_relaxed);
                    worked = true;
                }
                else
                {
                    errorFlag.store(true, std::memory_order_release);
                    active.store(false, std::memory_order_release);
                }
            }
            else if (!desiredOn.load(std::memory_order_relaxed))
            {
                drainFifo();
                closeFile();
                active.store(false, std::memory_order_release);
                worked = true;
            }
        }
        else if (desiredOn.load(std::memory_order_relaxed)
                 && !errorFlag.load(std::memory_order_relaxed))
        {
            if (openFile())
            {
                openedPathVersion = pathVersion.load(std::memory_order_relaxed);
                openSampleRate = sampleRate;
                active.store(true, std::memory_order_release);
                worked = true;
            }
            else
            {
                const juce::ScopedLock lock(pathLock);
                if (pathString.isNotEmpty())
                    errorFlag.store(true, std::memory_order_release);
                // Empty path: stay idle until the user picks a file.
            }
        }

        thread.wait(worked ? 1 : 5);
    }

    if (active.load(std::memory_order_relaxed))
    {
        drainFifo();
        closeFile();
        active.store(false, std::memory_order_release);
    }
}

} // namespace UTT

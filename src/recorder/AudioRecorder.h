#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace UTT {

// 24-bit WAV recorder with a background writer thread.
//
// Threading contract:
//  - audio thread:     setRecordEnabled() / process() - lock-free, allocation-free
//  - message thread:   setPath() / consumeError() / accessors
//  - writer thread:    owns every file object and all disk I/O
//
// The audio thread only pushes fixed-size stereo blocks into a preallocated
// single-producer/single-consumer ring; opening, writing and finalising the
// file happen on the writer thread. If the ring fills (disk stall) blocks are
// dropped and counted, never blocked on.
class AudioRecorder
{
public:
    AudioRecorder() = default;
    ~AudioRecorder();

    void prepare(double sr, int maxBlock);
    void reset();                              // request the current take to stop

    void setPath(const juce::String& path);    // message thread; live takes roll over to it
    juce::String getPath() const;

    void setRecordEnabled(bool enabled);       // audio thread, cheap
    void process(const float* const* channels, int numSamples); // audio thread, enqueue only

    bool isDesired() const { return desiredOn.load(std::memory_order_relaxed); }
    bool isActive() const  { return active.load(std::memory_order_acquire); }
    std::uint64_t getDroppedBlocks() const { return droppedBlocks.load(std::memory_order_relaxed); }
    bool consumeError();                       // message thread: true if a take failed

    static juce::File getDefaultPath()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                       .getChildFile("UTT Recordings");
        dir.createDirectory();
        auto ts = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        return dir.getNonexistentChildFile("UTT_" + ts, ".wav", false);
    }

private:
    struct Slot
    {
        std::vector<float> l, r;
        int count = 0;
    };

    class WriterThread : public juce::Thread
    {
    public:
        explicit WriterThread(AudioRecorder& ownerIn)
            : juce::Thread("UTT RecWriter"), owner(ownerIn) {}
        void run() override { owner.writerLoop(*this); }

    private:
        AudioRecorder& owner;
    };

    void writerLoop(juce::Thread& thread);
    bool drainFifo();
    bool openFile();
    void closeFile();
    void allocateFifo(int slots);

    // --- shared state ---
    std::atomic<bool> desiredOn { false };   // audio -> writer
    std::atomic<bool> active { false };      // writer -> audio (file open, accepting blocks)
    std::atomic<bool> errorFlag { false };   // writer -> message
    std::atomic<std::uint32_t> pathVersion { 0 }; // message -> writer (rollover request)
    std::atomic<std::uint64_t> droppedBlocks { 0 };

    mutable juce::CriticalSection pathLock;  // message <-> writer, never audio
    juce::String pathString;
    bool pathIsAutomatic = false;            // guarded by pathLock

    // --- writer-thread-only file state ---
    // NOTE: AudioFormatWriter deletes the OutputStream it was created with,
    // so the stream is never owned here after a successful open.
    std::uint32_t openedPathVersion = 0;
    double openSampleRate = 48000.0;
    std::unique_ptr<juce::WavAudioFormat> wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    // --- FIFO (audio produces, writer consumes) ---
    std::vector<Slot> fifo;
    std::uint32_t fifoMask = 0;
    int slotSamples = 256;
    std::atomic<std::uint32_t> readPos { 0 };
    std::atomic<std::uint32_t> writePos { 0 };

    double sampleRate = 48000.0;
    std::unique_ptr<WriterThread> writerThread;

    JUCE_DECLARE_NON_COPYABLE(AudioRecorder)
};

} // namespace UTT

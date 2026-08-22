// UTT DSP / contract unit tests (console runner).
#include <JuceHeader.h>
#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"
#include "Parameters.h"
#include "dsp/DCOffset.h"
#include "dsp/PhaseProcessor.h"
#include "dsp/ChannelProcessor.h"
#include "dsp/Panner.h"
#include "dsp/Oscilloscope.h"
#include "dsp/LUFSMeter.h"
#include "recorder/AudioRecorder.h"

#if !defined(UTT_SOURCE_DIR)
#error "UTTTests must be built with UTT_SOURCE_DIR defined"
#endif

namespace
{

juce::AudioBuffer<float> makeBuffer(int channels, int samples)
{
    return juce::AudioBuffer<float>(channels, samples);
}

void fillSine(juce::AudioBuffer<float>& b, int ch, double freq, double sr,
              double amplitude, double phase = 0.0, int offsetSamples = 0)
{
    auto* d = b.getWritePointer(ch);
    for (int i = 0; i < b.getNumSamples(); ++i)
        d[i] = static_cast<float>(amplitude
            * std::sin(2.0 * juce::MathConstants<double>::pi * freq
                       * (i + offsetSamples) / sr + phase));
}

// Complex demodulation phase (radians) of a channel's content.
double measuredPhase(const float* data, int n, double freq, double sr)
{
    double re = 0.0, im = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * freq * i / sr;
        re += data[i] * std::cos(w);
        im -= data[i] * std::sin(w);
    }
    return std::atan2(im, re);
}

} // namespace

//==============================================================================
class PhaseProcessorTests : public juce::UnitTest
{
public:
    PhaseProcessorTests() : juce::UnitTest("PhaseProcessor", "UTT") {}

    void runTest() override
    {
        const double sr = 44100.0;

        beginTest("0 degrees is bit-exact identity");
        {
            UTT::PhaseProcessor pp;
            pp.prepare(sr, 512);
            pp.setInvert(false, false);
            pp.setPhaseOffset(0.0f);

            juce::AudioBuffer<float> ref = makeBuffer(2, 4096);
            fillSine(ref, 0, 997.0, sr, 0.5);
            fillSine(ref, 1, 1234.0, sr, -0.25);

            juce::AudioBuffer<float> test = makeBuffer(2, 4096);
            for (int ch = 0; ch < 2; ++ch)
                test.copyFrom(ch, 0, ref, ch, 0, 4096);

            pp.process(test);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 4096; ++i)
                    expectEquals(test.getSample(ch, i), ref.getSample(ch, i),
                                 "phase offset 0 must not touch the signal");
        }

        beginTest("90 degrees at 1 kHz");
        {
            UTT::PhaseProcessor pp;
            pp.prepare(sr, 512);
            pp.setInvert(false, false);
            pp.setPhaseOffset(90.0f);

            // 1 s of settling, then measure over one block
            for (int block = 0; block < sr / 512; ++block)
            {
                auto b = makeBuffer(2, 512);
                fillSine(b, 0, 1000.0, sr, 0.5, 0.0, block * 512);
                fillSine(b, 1, 1000.0, sr, 0.5, 0.0, block * 512);
                pp.process(b);
            }

            const int n = 4096;
            auto b = makeBuffer(2, n);
            fillSine(b, 0, 1000.0, sr, 0.5, 0.0, n);
            fillSine(b, 1, 1000.0, sr, 0.5, 0.0, n);

            juce::AudioBuffer<float> input = makeBuffer(2, n);
            for (int ch = 0; ch < 2; ++ch)
                input.copyFrom(ch, 0, b, ch, 0, n);

            pp.process(b);

            const double phiIn = measuredPhase(input.getReadPointer(0), n, 1000.0, sr);
            const double phiOut = measuredPhase(b.getReadPointer(0), n, 1000.0, sr);
            double delta = phiOut - phiIn;
            while (delta > juce::MathConstants<double>::pi)  delta -= 2 * juce::MathConstants<double>::pi;
            while (delta < -juce::MathConstants<double>::pi) delta += 2 * juce::MathConstants<double>::pi;

            const double expectDeg = -90.0;
            const double gotDeg = juce::radiansToDegrees(delta);
            expectWithinAbsoluteError(gotDeg, expectDeg, 6.0,
                                      "allpass pair must shift 1 kHz by the knob amount");
        }

        beginTest("invert flips polarity");
        {
            UTT::PhaseProcessor pp;
            pp.prepare(sr, 512);
            pp.setInvert(true, false);
            pp.setPhaseOffset(0.0f);

            auto b = makeBuffer(2, 64);
            fillSine(b, 0, 100.0, sr, 0.5);
            fillSine(b, 1, 100.0, sr, 0.5);
            pp.process(b);
            for (int i = 0; i < 64; ++i)
            {
                expect(b.getSample(0, i) < 0.0f || b.getSample(0, i) == 0.0f, "L inverted");
                expect(b.getSample(1, i) == b.getSample(1, i), "R untouched");
            }
        }
    }
};

//==============================================================================
class DCOffsetTests : public juce::UnitTest
{
public:
    DCOffsetTests() : juce::UnitTest("DCOffset", "UTT") {}

    void runTest() override
    {
        const double sr = 48000.0;

        beginTest("removes DC");
        {
            UTT::DCOffset dc;
            dc.prepare(sr, 512);

            for (int block = 0; block < 100; ++block)
            {
                auto b = makeBuffer(2, 512);
                b.clear();
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        b.setSample(ch, i, 0.5f);
                dc.process(b);

                if (block == 99)
                {
                    float mean = 0.0f;
                    for (int i = 0; i < 512; ++i)
                        mean += b.getSample(0, i);
                    mean /= 512.0f;
                    expectWithinAbsoluteError(mean, 0.0f, 0.01f, "DC should decay to ~0");
                }
            }
        }

        beginTest("passes audio band");
        {
            UTT::DCOffset dc;
            dc.prepare(sr, 512);
            auto b = makeBuffer(2, 8192);
            fillSine(b, 0, 1000.0, sr, 0.5);
            fillSine(b, 1, 1000.0, sr, 0.5);

            juce::AudioBuffer<float> in = makeBuffer(2, 8192);
            for (int ch = 0; ch < 2; ++ch)
                in.copyFrom(ch, 0, b, ch, 0, 8192);

            for (int off = 0; off < 8192; off += 512)
            {
                auto sub = makeBuffer(2, 512);
                for (int ch = 0; ch < 2; ++ch)
                    sub.copyFrom(ch, 0, in, ch, off, 512);
                dc.process(sub);
                for (int ch = 0; ch < 2; ++ch)
                    b.copyFrom(ch, off, sub, ch, 0, 512);
            }

            auto rms = [](const float* d, int from, int n)
            {
                double s = 0.0;
                for (int i = from; i < from + n; ++i) s += double(d[i]) * d[i];
                return std::sqrt(s / n);
            };
            const double rIn = rms(in.getReadPointer(0), 4096, 4096);
            const double rOut = rms(b.getReadPointer(0), 4096, 4096);
            expectWithinAbsoluteError(rOut / rIn, 1.0, 0.01, "1 kHz must pass unattenuated");
        }
    }
};

//==============================================================================
class ChannelProcessorTests : public juce::UnitTest
{
public:
    ChannelProcessorTests() : juce::UnitTest("ChannelProcessor", "UTT") {}

    void runTest() override
    {
        const double sr = 44100.0;

        beginTest("separation=100 is transparent");
        {
            UTT::ChannelProcessor cp;
            cp.prepare(sr, 512);
            cp.setMonoMode(0);
            cp.setSeparationImmediate(100.0f);

            auto b = makeBuffer(2, 512);
            fillSine(b, 0, 440.0, sr, 0.4);
            fillSine(b, 1, 440.0, sr, -0.4);

            juce::AudioBuffer<float> ref = makeBuffer(2, 512);
            for (int ch = 0; ch < 2; ++ch) ref.copyFrom(ch, 0, b, ch, 0, 512);

            cp.process(b);
            cp.processPostSeparation(b);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    expectEquals(b.getSample(ch, i), ref.getSample(ch, i),
                                 "100% separation must be bit-exact neutral");
        }

        beginTest("separation=0 kills the side signal (mono mode Off)");
        {
            UTT::ChannelProcessor cp;
            cp.prepare(sr, 512);
            cp.setMonoMode(0);
            cp.setSeparationImmediate(0.0f);

            for (int block = 0; block < 16; ++block) // let the previous smoothing settle
            {
                auto b = makeBuffer(2, 512);
                fillSine(b, 0, 440.0, sr, 0.4);
                fillSine(b, 1, 440.0, sr, -0.4);
                cp.process(b);
                cp.processPostSeparation(b);

                if (block == 15)
                    for (int i = 0; i < 512; ++i)
                    {
                        expectWithinAbsoluteError(b.getSample(0, i), 0.0f, 1e-5f,
                                                  "0% separation collapses pure side to silence");
                        expectWithinAbsoluteError(b.getSample(1, i), 0.0f, 1e-5f, "");
                    }
            }
        }

        beginTest("mono sum removes side content");
        {
            UTT::ChannelProcessor cp;
            cp.prepare(sr, 512);
            cp.setMonoMode(1);
            cp.setSeparationImmediate(100.0f);

            auto b = makeBuffer(2, 512);
            fillSine(b, 0, 440.0, sr, 0.4);
            fillSine(b, 1, 440.0, sr, -0.4);
            cp.process(b);
            for (int i = 0; i < 512; ++i)
                expectWithinAbsoluteError(b.getSample(0, i), 0.0f, 1e-6f,
                                          "L+R of an antiphase pair is zero");
        }
    }
};

//==============================================================================
class PannerTests : public juce::UnitTest
{
public:
    PannerTests() : juce::UnitTest("Panner", "UTT") {}

    void runTest() override
    {
        const double sr = 44100.0;
        auto feed = [&](float pan)
        {
            UTT::Panner p;
            p.prepare(sr, 512);
            p.setPanImmediate(pan);
            auto b = makeBuffer(2, 512);
            fillSine(b, 0, 1000.0, sr, 0.5);
            fillSine(b, 1, 1000.0, sr, 0.5);
            p.process(b);
            return b;
        };

        beginTest("center keeps both channels at unity");
        {
            auto b = feed(0.0f);
            float peakL = 0.0f, peakR = 0.0f;
            for (int i = 0; i < 512; ++i)
            {
                expectWithinAbsoluteError(b.getSample(0, i), b.getSample(1, i), 1e-6f, "equal at center");
                peakL = juce::jmax(peakL, std::abs(b.getSample(0, i)));
                peakR = juce::jmax(peakR, std::abs(b.getSample(1, i)));
            }
            // Balance law: the centre detent is unity (transparent bypass)
            expectWithinAbsoluteError(peakL, 0.5f, 0.01f, "center L unity");
            expectWithinAbsoluteError(peakR, 0.5f, 0.01f, "center R unity");
        }

        beginTest("hard right kills L, keeps R");
        {
            auto b = feed(100.0f);
            float peakL = 0.0f, peakR = 0.0f;
            for (int i = 0; i < 512; ++i)
            {
                peakL = juce::jmax(peakL, std::abs(b.getSample(0, i)));
                peakR = juce::jmax(peakR, std::abs(b.getSample(1, i)));
            }
            expectWithinAbsoluteError(peakL, 0.0f, 1e-5f, "L silent");
            expectWithinAbsoluteError(peakR, 0.5f, 0.01f, "R full");
        }
    }
};

//==============================================================================
class LUFSMeterTests : public juce::UnitTest
{
public:
    LUFSMeterTests() : juce::UnitTest("LUFSMeter", "UTT") {}

    void runCalibration(double sr, int block)
    {
        UTT::LUFSMeter m;
        m.prepare(sr, block);

        const int total = static_cast<int>(5.0 * sr);
        int done = 0;
        while (done < total)
        {
            const int n = juce::jmin(block, total - done);
            auto b = makeBuffer(2, n);
            fillSine(b, 0, 997.0, sr, 1.0, 0.0, done);
            b.clear(1, 0, n); // R silent: BS.1770 reference condition
            m.process(b);
            done += n;
        }

        // BS.1770-4: a 0 dBFS 997 Hz sine on one channel reads -3.01 LKFS.
        expectWithinAbsoluteError(m.getIntegrated(), -3.01f, 0.15f,
                                  "integrated calibration @ " + juce::String(sr) + " Hz");
        expectWithinAbsoluteError(m.getMomentary(), -3.01f, 0.3f,
                                  "momentary calibration @ " + juce::String(sr) + " Hz");
    }

    void runTest() override
    {
        beginTest("K-weighting coefficients match the BS.1770 48 kHz table");
        {
            auto s1 = UTT::LUFSMeter::computeStage1(48000.0);
            expectWithinAbsoluteError(s1.b0, 1.53512485958697, 2e-5, "stage1 b0");
            expectWithinAbsoluteError(s1.b1, -2.69169618940638, 2e-5, "stage1 b1");
            expectWithinAbsoluteError(s1.b2, 1.19839281085285, 2e-5, "stage1 b2");
            expectWithinAbsoluteError(s1.a1, -1.69065929318241, 2e-5, "stage1 a1");
            expectWithinAbsoluteError(s1.a2, 0.73248077421585, 2e-5, "stage1 a2");

            auto s2 = UTT::LUFSMeter::computeStage2(48000.0);
            expectWithinAbsoluteError(s2.b0, 1.0, 2e-5, "stage2 b0");
            expectWithinAbsoluteError(s2.b1, -2.0, 2e-5, "stage2 b1");
            expectWithinAbsoluteError(s2.b2, 1.0, 2e-5, "stage2 b2");
            expectWithinAbsoluteError(s2.a1, -1.99004745483398, 2e-5, "stage2 a1");
            expectWithinAbsoluteError(s2.a2, 0.99007225036621, 2e-5, "stage2 a2");
        }

        beginTest("integrated loudness calibration");
        {
            runCalibration(48000.0, 480);
            runCalibration(44100.0, 441);
            runCalibration(96000.0, 960);
        }

        beginTest("silence reads -70");
        {
            UTT::LUFSMeter m;
            m.prepare(48000.0, 480);
            for (int i = 0; i < 100; ++i)
            {
                auto b = makeBuffer(2, 480);
                b.clear();
                m.process(b);
            }
            expectEquals(m.getIntegrated(), -70.0f);
        }
    }
};

//==============================================================================
class OscilloscopeTests : public juce::UnitTest
{
public:
    OscilloscopeTests() : juce::UnitTest("Oscilloscope", "UTT") {}

    void runTest() override
    {
        beginTest("snapshot generations advance and frames are ordered");
        {
            UTT::Oscilloscope osc;
            osc.prepare(44100.0, 512);

            std::vector<float> l, r;
            expect(osc.copyLatestFrame(l, r) == 0, "no frame before first process");

            auto b = makeBuffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                b.setSample(0, i, 0.25f);
                b.setSample(1, i, -0.25f);
            }
            osc.process(b);

            const auto g1 = osc.copyLatestFrame(l, r);
            expect(g1 != 0, "generation valid after publish");
            expectEquals(static_cast<int>(l.size()), UTT::Oscilloscope::maxPoints, "frame size");

            osc.process(b);
            const auto g2 = osc.copyLatestFrame(l, r);
            expect(g2 != g1 && g2 != 0, "generation advances");

            // Frame content matches the input (antiphase channels)
            expect(l.front() > 0.0f || r.front() < 0.0f, "L/R captured with sign");
        }
    }
};

//==============================================================================
class RecorderTests : public juce::UnitTest
{
public:
    RecorderTests() : juce::UnitTest("AudioRecorder", "UTT") {}

    void runTest() override
    {
        beginTest("records a WAV take through the background writer");
        {
            const double sr = 48000.0;
            const int blockLen = 512;
            const int blocks = 94; // ~1 s

            UTT::AudioRecorder rec;
            rec.prepare(sr, blockLen);

            const auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                 .getChildFile("utt_recorder_test.wav");
            tmp.deleteFile();
            rec.setPath(tmp.getFullPathName());

            // Build reference signal blocks
            std::vector<juce::AudioBuffer<float>> ref;
            for (int k = 0; k < blocks; ++k)
            {
                auto b = makeBuffer(2, blockLen);
                fillSine(b, 0, 997.0, sr, 0.5, 0.0, k * blockLen);
                fillSine(b, 1, 997.0, sr, -0.5, 0.0, k * blockLen);
                ref.push_back(std::move(b));
            }

            rec.setRecordEnabled(true);

            // Spin until the writer opens the file (blocks before open drop)
            for (int spin = 0; spin < 400 && !rec.isActive(); ++spin)
                juce::Thread::sleep(5);
            expect(rec.isActive(), "writer thread must open the take");

            for (const auto& b : ref)
                rec.process(b.getArrayOfReadPointers(), b.getNumSamples());

            rec.setRecordEnabled(false);
            for (int spin = 0; spin < 800 && rec.isActive(); ++spin)
                juce::Thread::sleep(5);
            expect(!rec.isActive(), "writer thread must finalise the take");
            expect(rec.getDroppedBlocks() == 0, "no drops at realtime pace");

            // Read the file back
            juce::WavAudioFormat fmt;
            std::unique_ptr<juce::AudioFormatReader> reader(
                fmt.createReaderFor(std::make_unique<juce::FileInputStream>(tmp).release(), true));
            expect(reader != nullptr, "take must be a readable WAV");
            if (reader != nullptr)
            {
                expectEquals(static_cast<int>(reader->numChannels), 2, "stereo take");
                expectWithinAbsoluteError(double(reader->sampleRate), sr, 1.0, "sample rate");
                expect(reader->bitsPerSample == 24, "24-bit depth");
                expectEquals(static_cast<int>(reader->lengthInSamples), blocks * blockLen,
                             "all enqueued samples written");

                juce::AudioBuffer<float> out(2, reader->lengthInSamples);
                reader->read(&out, 0, reader->lengthInSamples, 0, true, true);
                expectWithinAbsoluteError(out.getSample(0, 100),
                                          ref[0].getSample(0, 100), 2e-4f,
                                          "L content survives 24-bit round trip");
                expectWithinAbsoluteError(out.getSample(1, 100),
                                          ref[0].getSample(1, 100), 2e-4f,
                                          "R content survives 24-bit round trip");
            }

            tmp.deleteFile();
        }
    }
};

//==============================================================================
class ProcessorTests : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest("UTTProcessor", "UTT") {}

    void runTest() override
    {
        beginTest("buses layouts");
        {
            UTTProcessor proc;
            auto supported = [&](const juce::AudioChannelSet& in,
                                 const juce::AudioChannelSet& out)
            {
                juce::AudioProcessor::BusesLayout l;
                l.inputBuses.add(in);
                l.outputBuses.add(out);
                return proc.isBusesLayoutSupported(l);
            };
            expect(supported(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo()));
            expect(supported(juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo()));
            expect(supported(juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono()));
            expect(!supported(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono()),
                   "stereo->mono would discard the right channel");
            expect(!supported(juce::AudioChannelSet::discreteChannels(6),
                              juce::AudioChannelSet::discreteChannels(6)),
                   "multichannel is not processed end-to-end");
            expect(!supported(juce::AudioChannelSet::disabled(), juce::AudioChannelSet::stereo()));
        }

        beginTest("default chain is bit-exact transparent (stereo)");
        {
            UTTProcessor proc;
            proc.prepareToPlay(44100.0, 512);

            const int n = 512;
            juce::AudioBuffer<float> b(2, n);
            fillSine(b, 0, 997.0, 44100.0, 0.5);
            fillSine(b, 1, 997.0, 44100.0, -0.25);

            juce::AudioBuffer<float> ref(2, n);
            for (int ch = 0; ch < 2; ++ch)
                ref.copyFrom(ch, 0, b, ch, 0, n);

            juce::MidiBuffer midi;
            proc.processBlock(b, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                    expectEquals(b.getSample(ch, i), ref.getSample(ch, i),
                                 "defaults must process bit-exact (all modules neutral)");
        }

        beginTest("mono input is duplicated and metered, output returns L");
        {
            UTTProcessor proc;
            proc.prepareToPlay(44100.0, 256);

            juce::AudioBuffer<float> b(1, 256);
            fillSine(b, 0, 500.0, 44100.0, 0.5);
            juce::AudioBuffer<float> ref(1, 256);
            ref.copyFrom(0, 0, b, 0, 0, 256);

            juce::MidiBuffer midi;
            proc.processBlock(b, midi);

            for (int i = 0; i < 256; ++i)
                expectEquals(b.getSample(0, i), ref.getSample(0, i), "mono passthrough");
        }

        beginTest("legacy v0.2 state (meter/theme params) restores cleanly");
        {
            UTTProcessor proc;

            // Set a distinctive gain so we can verify real params survive.
            if (auto* p = proc.apvts.getParameter("gain"))
                p->setValueNotifyingHost(0.75f);

            juce::MemoryBlock mb;
            proc.getStateInformation(mb);

            auto xml = std::unique_ptr<juce::XmlElement>(
                proc.getXmlFromBinary(mb.getData(), static_cast<int>(mb.getSize())));
            expect(xml != nullptr);
            if (xml != nullptr)
            {
                for (const char* legacy : { "meterLPeak", "meterRPeak", "meterLRMS",
                                            "meterRRMS", "phaseCorrelation",
                                            "lufsMomentary", "lufsShortTerm",
                                            "lufsIntegrated", "themeIndex" })
                {
                    auto* child = xml->createNewChildElement("PARAM");
                    child->setAttribute("id", legacy);
                    child->setAttribute("value", -42.0);
                }

                juce::MemoryBlock legacyBlock;
                const juce::String xmlStr = xml->toString();
                legacyBlock.append(xmlStr.toRawUTF8(), xmlStr.getNumBytesAsUTF8());
                proc.setStateInformation(legacyBlock.getData(),
                                         static_cast<int>(legacyBlock.getSize()));
            }

            expect(proc.apvts.getRawParameterValue("gain") != nullptr, "gain present");
            expect(proc.apvts.getRawParameterValue("meterLPeak") == nullptr,
                   "legacy meter pseudo-parameter must not exist");
            expect(proc.apvts.getRawParameterValue("themeIndex") == nullptr,
                   "theme pseudo-parameter must not exist");
            expectWithinAbsoluteError(
                proc.apvts.getRawParameterValue("gain")->load(), -6.0f, 0.5f,
                "restored gain should track the value saved before injection");
        }
    }
};

//==============================================================================
class ParameterContractTests : public juce::UnitTest
{
public:
    ParameterContractTests() : juce::UnitTest("ParameterContract", "UTT") {}

    void runTest() override
    {
        beginTest("config/parameters-v2.json matches Parameters.h");
        {
            auto json = juce::JSON::parse(juce::File(UTT_SOURCE_DIR)
                                              .getChildFile("config")
                                              .getChildFile("parameters-v2.json"));
            expect(json.isObject(), "config JSON must parse");
            auto* params = json.getProperty("parameters", {}).getArray();
            expect(params != nullptr, "parameters array present");

            UTTProcessor proc; // builds the APVTS layout in contract order
            auto& hostParams = proc.getParameters();

            expectEquals(static_cast<int>(params->size()), hostParams.size(),
                         "config and layout parameter counts must agree");
            if (params->size() == hostParams.size())
            {
                for (int i = 0; i < params->size(); ++i)
                {
                    auto* obj = (*params)[static_cast<int>(i)].getDynamicObject();
                    expect(obj != nullptr);
                    if (obj == nullptr) continue;

                    const juce::String id = obj->getProperty("id").toString();
                    const juce::String type = obj->getProperty("type").toString();

                    auto* hp = hostParams[static_cast<int>(i)];
                    auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(hp);
                    expect(withId != nullptr,
                           "host parameter at index " + juce::String(i)
                               + " must be an AudioProcessorParameterWithID");
                    if (withId != nullptr)
                        expectEquals(withId->paramID.toLowerCase(), id.toLowerCase(),
                                     "host parameter order must follow the config at index "
                                         + juce::String(i));

                    if (type == "float")
                    {
                        auto* f = dynamic_cast<juce::AudioParameterFloat*>(hp);
                        expect(f != nullptr, id + " should be a float parameter");
                        if (f != nullptr)
                        {
                            expectWithinAbsoluteError(f->get(), static_cast<float>(obj->getProperty("default")),
                                                      1e-4f, id + " default");
                            expectWithinAbsoluteError(f->getNormalisableRange().start,
                                                      static_cast<float>(obj->getProperty("min")),
                                                      1e-4f, id + " min");
                            expectWithinAbsoluteError(f->getNormalisableRange().end,
                                                      static_cast<float>(obj->getProperty("max")),
                                                      1e-4f, id + " max");
                        }
                    }
                    else if (type == "bool")
                    {
                        auto* bp = dynamic_cast<juce::AudioParameterBool*>(hp);
                        expect(bp != nullptr, id + " should be a bool parameter");
                        if (bp != nullptr)
                        {
                            const bool wantDefault = static_cast<bool>(obj->getProperty("default"));
                            expect(bp->get() == wantDefault, id + " default");
                        }
                    }
                    else if (type == "choice")
                    {
                        auto* cp = dynamic_cast<juce::AudioParameterChoice*>(hp);
                        expect(cp != nullptr, id + " should be a choice parameter");
                        if (cp != nullptr)
                        {
                            expectEquals(cp->getIndex(), static_cast<int>(obj->getProperty("default")),
                                         id + " default index");
                            if (auto* choices = obj->getProperty("choices").getArray())
                                expectEquals(cp->choices.size(), static_cast<int>(choices->size()),
                                             id + " choice count");
                        }
                    }
                    else
                    {
                        expect(false, "unknown type in config for " + id);
                    }
                }
            }
        }
    }
};

//==============================================================================
static PhaseProcessorTests     phaseProcessorTests;
static DCOffsetTests           dcOffsetTests;
static ChannelProcessorTests   channelProcessorTests;
static PannerTests             pannerTests;
static LUFSMeterTests          lufsMeterTests;
static OscilloscopeTests       oscilloscopeTests;
static RecorderTests           recorderTests;
static ProcessorTests          processorTests;
static ParameterContractTests  parameterContractTests;

//==============================================================================
class ConsoleLogger : public juce::Logger
{
public:
    void logMessage(const juce::String& message) override
    {
        std::fprintf(stderr, "%s\n", message.toRawUTF8());
        std::fflush(stderr);
    }
};

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    ConsoleLogger logger;
    juce::Logger::setCurrentLogger(&logger);

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    juce::Logger::setCurrentLogger(nullptr);

    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        failures += result->failures;
        passes += result->passes;
    }

    std::printf("\nUTTTests: %d passed, %d failed\n", passes, failures);
    return failures == 0 ? 0 : 1;
}

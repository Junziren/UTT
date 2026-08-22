#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UTTWebView.h"
#include "BinaryData.h"

#include <cmath>

namespace
{
constexpr int timerHz   = 25;   // meter transport
constexpr int scopeDiv  = 2;    // scope every 2nd tick (~12 Hz)
constexpr int scopePts  = 256;  // decimated points sent to JS
} // namespace

UTTWebView::UTTWebView (UTTProcessor& p, juce::AudioProcessorEditor& ownerEditor)
    : juce::WebBrowserComponent (
        juce::WebBrowserComponent::Options()
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withNativeFunction ("uttUiReady",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    juce::ignoreUnused (params);
                    completion (buildSnapshot());
                })
            .withNativeFunction ("uttGesture",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    if (params.size() >= 1)
                        handleGesture (params.getReference (0), completion);
                    else
                        completion (juce::var());
                })
            .withNativeFunction ("uttResetParam",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    if (params.size() >= 1)
                        handleResetParam (params.getReference (0));
                    completion (juce::var (true));
                })
            .withNativeFunction ("uttRecordBrowse",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    juce::ignoreUnused (params);
                    handleRecordBrowse();
                    completion (juce::var (true));
                })
            .withNativeFunction ("uttAbout",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    juce::ignoreUnused (params);
                    handleAbout();
                    completion (juce::var (true));
                })
            .withNativeFunction ("uttParamMenu",
                [this] (const juce::Array<juce::var>& params,
                        const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
                {
                    if (params.size() >= 1)
                        handleParamMenu (params.getReference (0));
                    completion (juce::var (true));
                })
            .withEventListener ("uttParamState", [] (const juce::var&) {})
            .withEventListener ("uttMeter",      [] (const juce::var&) {})
            .withEventListener ("uttScope",      [] (const juce::var&) {})
            .withEventListener ("uttRecord",     [] (const juce::var&) {})
            .withResourceProvider (
                [] (const juce::String& url)
                {
                    // The provider receives "/" for the root document and
                    // "/name.ext" for linked resources. BinaryData symbol
                    // names replace non-alphanumerics with underscores.
                    auto path = url.upToFirstOccurrenceOf ("?", false, false);
                    if (path == "/" || path.isEmpty())
                        path = "/index.html";

                    struct Entry { const char* urlName; const char* binName; const char* mime; };
                    static const Entry entries[] = {
                        { "/index.html", "index_html", "text/html" },
                        { "/utt.css",    "utt_css",    "text/css" },
                        { "/utt.js",     "utt_js",     "text/javascript" },
                    };

                    for (auto& e : entries)
                    {
                        if (path != e.urlName)
                            continue;

                        int size = 0;
                        if (auto* data = BinaryData::getNamedResource (
                                (juce::String ("BinaryData_") + e.binName).toRawUTF8(), size))
                        {
                            juce::WebBrowserComponent::Resource resource;
                            resource.data.assign (reinterpret_cast<const std::byte*> (data),
                                                  reinterpret_cast<const std::byte*> (data) + size);
                            resource.mimeType = juce::String (e.mime);
                            return std::make_optional (std::move (resource));
                        }
                    }
                    return std::optional<juce::WebBrowserComponent::Resource>();
                })),
      processor (p),
      apvts (p.apvts),
      editor (ownerEditor)
{
    scopeL.assign ((size_t) UTT::Oscilloscope::maxPoints, 0.0f);
    scopeR.assign ((size_t) UTT::Oscilloscope::maxPoints, 0.0f);
}

UTTWebView::~UTTWebView()
{
    stopTimer();
    for (auto& id : listenedParams)
        apvts.removeParameterListener (id, this);
}

void UTTWebView::loadInterface()
{
    // listen to every parameter (message-thread callbacks)
    for (auto* param : processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            apvts.addParameterListener (withId->paramID, this);
            listenedParams.add (withId->paramID);
        }

    goToURL (getResourceProviderRoot() + "/index.html");
    startTimerHz (timerHz);
}

void UTTWebView::setBridgeActive (bool active)
{
    bridgeActive = active;
    if (! active)
        stopTimer();
    else
        startTimerHz (timerHz);
}

//==============================================================================
void UTTWebView::parameterChanged (const juce::String& parameterID, float)
{
    if (! dirtyParams.contains (parameterID))
        dirtyParams.add (parameterID);
}

void UTTWebView::handleGesture (const juce::var& payload,
                                const juce::WebBrowserComponent::NativeFunctionCompletion& completion)
{
    auto* obj = payload.getDynamicObject();
    if (obj == nullptr) { completion (juce::var (false)); return; }

    const auto id = obj->getProperty ("id").toString();
    const auto phase = obj->getProperty ("phase").toString();

    auto* param = apvts.getParameter (id);
    if (param == nullptr) { completion (juce::var (false)); return; }

    if (phase == "begin")
    {
        param->beginChangeGesture();
    }
    else if (phase == "set")
    {
        auto v = (float) (double) obj->getProperty ("value");
        if (std::isfinite (v))
            param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v));
    }
    else if (phase == "end")
    {
        param->endChangeGesture();
    }
    completion (juce::var (true));
}

void UTTWebView::handleResetParam (const juce::var& payload)
{
    if (auto* obj = payload.getDynamicObject())
        if (auto* param = apvts.getParameter (obj->getProperty ("id").toString()))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->getDefaultValue());
            param->endChangeGesture();
        }
}

void UTTWebView::handleRecordBrowse()
{
    auto defaultFile = UTT::AudioRecorder::getDefaultPath();
    auto chooser = std::make_shared<juce::FileChooser> (
        "选择录制文件", defaultFile.getParentDirectory(), "*.wav", true);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc)
        {
            if (fc.getResult() == juce::File())
                return;
            processor.setRecordPath (fc.getResult().getFullPathName());
            pushRecordState();
        });
}

void UTTWebView::handleAbout()
{
    juce::MessageBoxOptions options;
    options = options.withTitle ("关于 UTT")
                 .withMessage ("UTT — Utility Toolbox v0.3.0\n"
                               "开发者：UnpureBloom\n"
                               "通道条工具 VST3 / AUv3（预备）\n\n"
                               "JUCE 8 · WebView2 界面 · GPLv3\n"
                               "Bilibili: space.bilibili.com/227573145")
                 .withIconType (juce::MessageBoxIconType::InfoIcon)
                 .withButton ("关闭");
    juce::NativeMessageBox::showAsync (options, nullptr);
}

void UTTWebView::handleParamMenu (const juce::var& payload)
{
    if (auto* obj = payload.getDynamicObject())
    {
        auto* param = apvts.getParameter (obj->getProperty ("id").toString());
        if (param == nullptr)
            return;

        auto* hostContext = editor.getHostContext();
        if (hostContext == nullptr)
            return;

        if (auto menu = hostContext->getContextMenuForParameter (param))
            menu->showNativeMenu (editor.localPointToGlobal (editor.getMouseXYRelative()));
    }
}

//==============================================================================
juce::String UTTWebView::formatParameterValue (const juce::AudioProcessorParameter& param)
{
    if (auto* withId = dynamic_cast<const juce::AudioProcessorParameterWithID*> (&param))
    {
        const auto& id = withId->paramID;

        if (auto* choice = dynamic_cast<const juce::AudioParameterChoice*> (&param))
            return choice->getCurrentChoiceName();

        if (auto* floatParam = dynamic_cast<const juce::AudioParameterFloat*> (&param))
        {
            const double v = floatParam->get();
            if (id == "phaseOffset")
                return juce::String (juce::roundToInt (v)) + juce::String::fromUTF8 ("\xc2\xb0");
            if (id == "stereoSeparation")
                return juce::String (juce::roundToInt (v)) + "%";
            if (id == "pan")
            {
                auto pc = juce::roundToInt (v);
                if (pc == 0) return "C";
                return pc < 0 ? ("L" + juce::String (-pc)) : ("R" + juce::String (pc));
            }
            // gain-style dB params
            return (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB";
        }
    }
    return juce::String ();
}

juce::var UTTWebView::buildSnapshot()
{
    auto* root = new juce::DynamicObject();
    juce::Array<juce::var> params;

    for (auto* param : processor.getParameters())
    {
        auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param);
        if (withId == nullptr)
            continue;

        auto* po = new juce::DynamicObject();
        po->setProperty ("id", withId->paramID);
        po->setProperty ("v", param->getValue());
        po->setProperty ("t", formatParameterValue (*param));
        po->setProperty ("d", param->getDefaultValue());
        params.add (juce::var (po));
    }
    root->setProperty ("params", params);

    auto* rec = new juce::DynamicObject();
    rec->setProperty ("on", processor.getRecorder().isDesired());
    rec->setProperty ("active", processor.getRecorder().isActive());
    rec->setProperty ("path", processor.getRecordPath());
    rec->setProperty ("dropped", (juce::int64) processor.getRecorder().getDroppedBlocks());
    root->setProperty ("record", juce::var (rec));

    auto sr = processor.getSampleRate();
    if (sr > 0)
    {
        juce::String engine = juce::String (juce::roundToInt (sr));
        engine += " Hz \xc2\xb7 ";
        engine += juce::String (processor.getBlockSize());
        root->setProperty ("engine", engine);
    }
    else
    {
        root->setProperty ("engine", juce::String ());
    }

    lastRecordOn = processor.getRecorder().isDesired();
    lastRecordActive = processor.getRecorder().isActive();
    lastRecordPath = processor.getRecordPath();

    return juce::var (root);
}

//==============================================================================
void UTTWebView::timerCallback()
{
    ++tick;

    if (! bridgeActive)
        return;

    pushMeterFrame();
    if (tick % scopeDiv == 0)
        pushScopeFrame();
    flushDirtyParams();

    // record state: push when something changed (or a write error appeared)
    auto& rec = processor.getRecorder();
    const bool on = rec.isDesired();
    const bool active = rec.isActive();
    auto dropped = rec.getDroppedBlocks();
    auto path = processor.getRecordPath();

    if (rec.consumeError())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("error", true);
        obj->setProperty ("on", on);
        obj->setProperty ("active", false);
        obj->setProperty ("path", path);
        obj->setProperty ("dropped", (juce::int64) dropped);
        emitEventIfBrowserIsVisible ("uttRecord", juce::var (obj));
    }
    else if (on != lastRecordOn || active != lastRecordActive || path != lastRecordPath)
    {
        lastRecordOn = on;
        lastRecordActive = active;
        lastRecordPath = path;
        pushRecordState();
    }
    else if (dropped != lastDroppedSeen)
    {
        lastDroppedSeen = dropped;
        pushRecordState();
    }
}

void UTTWebView::flushDirtyParams()
{
    if (dirtyParams.isEmpty())
        return;

    auto* root = new juce::DynamicObject();
    auto* params = new juce::DynamicObject();

    for (auto& id : dirtyParams)
    {
        if (auto* param = apvts.getParameter (id))
        {
            auto* po = new juce::DynamicObject();
            po->setProperty ("v", param->getValue());
            po->setProperty ("t", formatParameterValue (*param));
            params->setProperty (id, juce::var (po));
        }
    }
    dirtyParams.clearQuick();
    root->setProperty ("params", juce::var (params));
    emitEventIfBrowserIsVisible ("uttParamState", juce::var (root));
}

void UTTWebView::pushMeterFrame()
{
    const auto& m = processor.getMetering();
    const auto& pm = processor.getPhaseMeter();
    const auto& lufs = processor.getLUFSMeter();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("lp", m.getLPeak());
    obj->setProperty ("rp", m.getRPeak());
    obj->setProperty ("lr", m.getLRMS());
    obj->setProperty ("rr", m.getRRMS());
    obj->setProperty ("corr", pm.getCorrelation());
    obj->setProperty ("lm", lufs.getMomentary());
    obj->setProperty ("ls", lufs.getShortTerm());
    obj->setProperty ("li", lufs.getIntegrated());
    emitEventIfBrowserIsVisible ("uttMeter", juce::var (obj));
}

void UTTWebView::pushScopeFrame()
{
    const auto& scope = processor.getOscilloscope();
    auto generation = scope.copyLatestFrame (scopeL, scopeR);
    if (generation == 0 || generation == lastScopeGeneration)
        return;
    lastScopeGeneration = generation;

    const int n = juce::jmin ((int) scopeL.size(), (int) scopeR.size());
    if (n < 2)
        return;

    juce::Array<juce::var> lArr, rArr;
    lArr.ensureStorageAllocated (scopePts);
    rArr.ensureStorageAllocated (scopePts);
    const double stride = (double) (n - 1) / (scopePts - 1);
    for (int i = 0; i < scopePts; ++i)
    {
        auto idx = juce::roundToInt (i * stride);
        lArr.add (scopeL[(size_t) idx]);
        rArr.add (scopeR[(size_t) idx]);
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("g", (juce::int64) generation);
    obj->setProperty ("l", lArr);
    obj->setProperty ("r", rArr);
    emitEventIfBrowserIsVisible ("uttScope", juce::var (obj));
}

void UTTWebView::pushRecordState()
{
    auto& rec = processor.getRecorder();
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("on", rec.isDesired());
    obj->setProperty ("active", rec.isActive());
    obj->setProperty ("path", processor.getRecordPath());
    obj->setProperty ("dropped", (juce::int64) rec.getDroppedBlocks());
    obj->setProperty ("error", false);
    emitEventIfBrowserIsVisible ("uttRecord", juce::var (obj));
}

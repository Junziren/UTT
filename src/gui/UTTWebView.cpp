#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UTTWebView.h"
#include "BinaryData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
constexpr int timerHz = 20;
constexpr int scopeDivisor = 2;
constexpr int scopePoints = 256;
constexpr juce::uint32 readyTimeoutMs = 5000;
constexpr char uiBuildPlaceholder[] = "__UTT_UI_BUILD_ID__";

constexpr std::array<const char*, 18> parameterIDs {
    "dcOffsetBypass", "phaseInvertL", "phaseInvertR", "phaseOffset",
    "gain", "msMode", "monoMode", "stereoSeparation", "monoToStereo",
    "channelSwap", "pan", "outputLGain", "outputRGain", "outputLMute",
    "outputRMute", "outputLSolo", "outputRSolo", "recordEnable"
};

constexpr char nativeHardeningScript[] = R"JS(
(() => {
  const editable = (node) => !!(node && node.closest
    && node.closest('input, textarea, select, [contenteditable="true"]'));

  document.addEventListener('contextmenu', (event) => event.preventDefault(), true);
  document.addEventListener('dragstart', (event) => event.preventDefault(), true);
  document.addEventListener('dragover', (event) => event.preventDefault(), true);
  document.addEventListener('drop', (event) => event.preventDefault(), true);
  document.addEventListener('selectstart', (event) => {
    if (!editable(event.target)) event.preventDefault();
  }, true);

  window.addEventListener('keydown', (event) => {
    const key = String(event.key || '').toLowerCase();
    if (key === 'f5' || ((event.ctrlKey || event.metaKey)
        && ['+', '-', '=', '0', 'r', 'u', 's'].includes(key))) {
      event.preventDefault();
      event.stopPropagation();
    }
  }, true);

  window.addEventListener('wheel', (event) => {
    if (event.ctrlKey) event.preventDefault();
  }, { passive: false, capture: true });
})();
)JS";

bool isAllowedParameterID (const juce::String& parameterID)
{
    return std::any_of (parameterIDs.begin(), parameterIDs.end(), [&] (const char* candidate)
    {
        return parameterID == candidate;
    });
}

bool parseFiniteNumber (const juce::var& value, double& result)
{
    if (! (value.isInt() || value.isInt64() || value.isDouble()))
        return false;

    result = static_cast<double> (value);
    return std::isfinite (result);
}

juce::String unitForParameter (const juce::String& parameterID)
{
    if (parameterID == "phaseOffset")
        return juce::String::fromUTF8 ("\xc2\xb0");
    if (parameterID == "stereoSeparation")
        return "%";
    if (parameterID == "gain" || parameterID == "outputLGain" || parameterID == "outputRGain")
        return "dB";
    return {};
}
} // namespace

class UTTWebView::Browser final : public juce::WebBrowserComponent
{
public:
    Browser (UTTWebView& ownerIn, juce::WebBrowserComponent::Options options)
        : juce::WebBrowserComponent (std::move (options)), owner (ownerIn)
    {
    }

    bool pageAboutToLoad (const juce::String& url) override
    {
        return url == "about:blank"
            || url.startsWithIgnoreCase (juce::WebBrowserComponent::getResourceProviderRoot());
    }

    void pageFinishedLoading (const juce::String& url) override
    {
        owner.pageFinishedLoading (url);
    }

    bool pageLoadHadNetworkError (const juce::String& errorInfo) override
    {
        owner.pageLoadFailed (errorInfo);
        return true;
    }

private:
    UTTWebView& owner;
};

UTTWebView::UTTWebView (UTTProcessor& p,
                        juce::AudioProcessorEditor& ownerEditor,
                        FailureHandler onFailure,
                        ReadyHandler onReady)
    : processor (p),
      apvts (p.apvts),
      editor (ownerEditor),
      failureHandler (std::move (onFailure)),
      readyHandler (std::move (onReady))
{
    scopeL.assign ((size_t) UTT::Oscilloscope::maxPoints, 0.0f);
    scopeR.assign ((size_t) UTT::Oscilloscope::maxPoints, 0.0f);

    if (! juce::WebBrowserComponent::areOptionsSupported (makeBrowserOptions (*this)))
    {
        reportFailure (SurfaceState::unavailable, "WebView2 runtime is unavailable.");
        return;
    }

    browser = std::make_unique<Browser> (*this, makeBrowserOptions (*this));
    addAndMakeVisible (*browser);
}

UTTWebView::~UTTWebView()
{
    stopTimer();
    endActiveParameterGestures();

    for (const auto& id : listenedParameters)
        apvts.removeParameterListener (id, this);

    browser.reset();
}

juce::WebBrowserComponent::Options UTTWebView::makeBrowserOptions (UTTWebView& owner)
{
    auto options = juce::WebBrowserComponent::Options {};

#if JUCE_WINDOWS
    const auto userDataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("UnpureBloom")
        .getChildFile ("UTT-WebView2");

    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
            .withUserDataFolder (userDataFolder)
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled()
            .withBackgroundColour (juce::Colour (0xfffafaf8)));
#endif

    return options
        .withWebViewLifetimeListener (&owner)
        .withNativeIntegrationEnabled()
        .withUserScript (nativeHardeningScript)
        .withResourceProvider ([] (const juce::String& url)
        {
            return getResource (url);
        })
        .withEventListener ("uiReady", [&owner] (const juce::var&)
        {
            owner.handleUiReady();
        })
        .withEventListener ("parameterGesture", [&owner] (const juce::var& payload)
        {
            owner.handleParameterGesture (payload);
        })
        .withEventListener ("parameterReset", [&owner] (const juce::var& payload)
        {
            owner.handleParameterReset (payload);
        })
        .withEventListener ("parameterContext", [&owner] (const juce::var& payload)
        {
            owner.handleParameterContext (payload);
        })
        .withEventListener ("recordBrowse", [&owner] (const juce::var&)
        {
            owner.handleRecordBrowse();
        });
}

juce::String UTTWebView::getMimeType (const juce::String& path)
{
    if (path.endsWithIgnoreCase (".html")) return "text/html";
    if (path.endsWithIgnoreCase (".css"))  return "text/css";
    if (path.endsWithIgnoreCase (".js"))   return "text/javascript";
    return "application/octet-stream";
}

UTTWebView::ResourceResult UTTWebView::getResource (const juce::String& url)
{
    auto requestedPath = url.upToFirstOccurrenceOf ("?", false, false);
    requestedPath = requestedPath.trimCharactersAtStart ("/");
    if (requestedPath.isEmpty())
        requestedPath = "index.html";

    static const juce::StringArray allowedResources { "index.html", "styles.css", "app.js" };
    if (! allowedResources.contains (requestedPath)
        || requestedPath.contains ("..")
        || requestedPath.containsChar ('\\')
        || requestedPath.containsChar ('/'))
        return std::nullopt;

    for (int index = 0; index < BinaryData::namedResourceListSize; ++index)
    {
        const auto* resourceName = BinaryData::namedResourceList[index];
        const auto originalFilename = juce::File (BinaryData::getNamedResourceOriginalFilename (resourceName)).getFileName();
        if (! originalFilename.equalsIgnoreCase (requestedPath))
            continue;

        int dataSize = 0;
        const auto* data = BinaryData::getNamedResource (resourceName, dataSize);
        if (data == nullptr || dataSize <= 0)
            return std::nullopt;

        Resource resource;

        if (requestedPath == "index.html")
        {
            std::string document (static_cast<const char*> (data), (size_t) dataSize);
            const std::string buildID (UTT_UI_BUILD_ID);
            size_t position = 0;

            while ((position = document.find (uiBuildPlaceholder, position)) != std::string::npos)
            {
                document.replace (position, std::strlen (uiBuildPlaceholder), buildID);
                position += buildID.length();
            }

            resource.data.resize (document.size());
            std::memcpy (resource.data.data(), document.data(), document.size());
        }
        else
        {
            resource.data.resize ((size_t) dataSize);
            std::memcpy (resource.data.data(), data, (size_t) dataSize);
        }

        resource.mimeType = getMimeType (requestedPath);
        return resource;
    }

    return std::nullopt;
}

void UTTWebView::webViewConstructed (juce::WebBrowserComponent* webView)
{
    constructedBrowser = webView;
}

void UTTWebView::webViewDestructed (juce::WebBrowserComponent* webView)
{
    if (constructedBrowser == webView)
        constructedBrowser = nullptr;

    endActiveParameterGestures();
}

void UTTWebView::loadInterface()
{
    if (interfaceRequested || browser == nullptr || failureReported)
        return;

    for (auto* parameter : processor.getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter))
        {
            apvts.addParameterListener (withID->paramID, this);
            listenedParameters.addIfNotAlreadyThere (withID->paramID);
        }

    interfaceRequested = true;
    surfaceState = SurfaceState::loading;
    readyDeadlineMs = juce::Time::getMillisecondCounter() + readyTimeoutMs;
    browser->goToURL (juce::WebBrowserComponent::getResourceProviderRoot()
                      + "index.html?build=" + juce::String (UTT_UI_BUILD_ID));

    // Keep the watchdog alive even while the native loading panel is frontmost.
    startTimerHz (timerHz);
}

void UTTWebView::resized()
{
    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

void UTTWebView::visibilityChanged()
{
    setSurfaceActive (isShowing());
}

void UTTWebView::timerCallback()
{
    if (! frontendReady)
    {
        if (interfaceRequested && ! failureReported
            && juce::Time::getMillisecondCounter() >= readyDeadlineMs)
            reportFailure (SurfaceState::loadError,
                           "Web UI did not signal ready within 5 seconds.");
        return;
    }

    if (! surfaceActive)
        return;

    ++tick;
    pushMeterFrame();
    if (tick % scopeDivisor == 0)
        pushScopeFrame();
    flushDirtyParameters();

    auto& recorder = processor.getRecorder();
    if (recorder.consumeError())
        recordErrorPending = true;

    const auto on = recorder.isDesired();
    const auto active = recorder.isActive();
    const auto dropped = recorder.getDroppedBlocks();
    const auto path = processor.getRecordPath();

    if (recordErrorPending || on != lastRecordOn || active != lastRecordActive
        || dropped != lastDroppedSeen || path != lastRecordPath)
        pushRecordState();
}

void UTTWebView::parameterChanged (const juce::String& parameterID, float)
{
    if (! dirtyParameters.contains (parameterID))
        dirtyParameters.add (parameterID);
}

void UTTWebView::handleUiReady()
{
    if (failureReported)
        return;

    frontendReady = true;
    surfaceState = SurfaceState::ready;

    if (readyHandler)
        readyHandler();

    dirtyParameters.clearQuick();
    sendFullState();

    auto* visibility = new juce::DynamicObject();
    visibility->setProperty ("active", surfaceActive);
    emitEvent ("surfaceVisibility", juce::var (visibility));
}

void UTTWebView::handleParameterGesture (const juce::var& payload)
{
    auto* object = payload.getDynamicObject();
    if (object == nullptr)
        return;

    const auto parameterID = object->getProperty ("id").toString();
    const auto phase = object->getProperty ("phase").toString();
    if (parameterID.length() > 64 || ! isAllowedParameterID (parameterID)
        || (phase != "begin" && phase != "value" && phase != "end"))
        return;

    auto* parameter = apvts.getParameter (parameterID);
    if (parameter == nullptr)
        return;

    if (phase == "begin")
    {
        if (! activeParameterGestures.contains (parameter))
        {
            parameter->beginChangeGesture();
            activeParameterGestures.add (parameter);
        }
        lastTouchedParameterID = parameterID;
        return;
    }

    if (phase == "end")
    {
        const auto index = activeParameterGestures.indexOf (parameter);
        if (index >= 0)
        {
            parameter->endChangeGesture();
            activeParameterGestures.remove (index);
        }
        return;
    }

    double normalized = 0.0;
    if (! parseFiniteNumber (object->getProperty ("normalized"), normalized))
        return;

    if (! activeParameterGestures.contains (parameter))
    {
        parameter->beginChangeGesture();
        activeParameterGestures.add (parameter);
    }

    lastTouchedParameterID = parameterID;
    parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (float) normalized));
}

void UTTWebView::handleParameterReset (const juce::var& payload)
{
    auto* object = payload.getDynamicObject();
    if (object == nullptr)
        return;

    const auto parameterID = object->getProperty ("id").toString();
    if (parameterID.length() > 64 || ! isAllowedParameterID (parameterID))
        return;

    if (auto* parameter = apvts.getParameter (parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->getDefaultValue());
        parameter->endChangeGesture();
        lastTouchedParameterID = parameterID;
    }
}

void UTTWebView::handleParameterContext (const juce::var& payload)
{
    auto* object = payload.getDynamicObject();
    if (object == nullptr)
        return;

    const auto parameterID = object->getProperty ("id").toString();
    if (parameterID.length() > 64 || ! isAllowedParameterID (parameterID))
        return;

    auto* parameter = apvts.getParameter (parameterID);
    auto* hostContext = editor.getHostContext();
    if (parameter == nullptr || hostContext == nullptr)
        return;

    if (auto menu = hostContext->getContextMenuForParameter (parameter))
        menu->showNativeMenu (editor.localPointToGlobal (editor.getMouseXYRelative()));
}

void UTTWebView::handleRecordBrowse()
{
    const auto defaultFile = UTT::AudioRecorder::getDefaultPath();
    auto chooser = std::make_shared<juce::FileChooser> (
        "Choose recording file", defaultFile.getParentDirectory(), "*.wav", true);
    auto safeThis = juce::Component::SafePointer<UTTWebView> (this);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                             | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, chooser] (const juce::FileChooser& fileChooser)
        {
            if (safeThis == nullptr || fileChooser.getResult() == juce::File())
                return;

            safeThis->processor.setRecordPath (fileChooser.getResult().getFullPathName());
            safeThis->pushRecordState();
        });
}

juce::var UTTWebView::describeParameter (const juce::AudioProcessorParameter& parameter) const
{
    auto* withID = dynamic_cast<const juce::AudioProcessorParameterWithID*> (&parameter);
    if (withID == nullptr)
        return {};

    auto* object = new juce::DynamicObject();
    const auto id = withID->paramID;

    object->setProperty ("id", id);
    object->setProperty ("normalized", parameter.getValue());
    object->setProperty ("defaultNormalized", parameter.getDefaultValue());
    object->setProperty ("text", formatParameterValue (parameter));
    object->setProperty ("unit", unitForParameter (id));

    if (auto* boolParameter = dynamic_cast<const juce::AudioParameterBool*> (&parameter))
    {
        object->setProperty ("kind", "toggle");
        object->setProperty ("plain", boolParameter->get() ? 1 : 0);
    }
    else if (auto* choiceParameter = dynamic_cast<const juce::AudioParameterChoice*> (&parameter))
    {
        object->setProperty ("kind", "choice");
        object->setProperty ("plain", choiceParameter->getIndex());

        juce::Array<juce::var> choices;
        for (const auto& choice : choiceParameter->choices)
            choices.add (choice);
        object->setProperty ("choices", choices);
    }
    else if (auto* floatParameter = dynamic_cast<const juce::AudioParameterFloat*> (&parameter))
    {
        object->setProperty ("kind", "continuous");
        object->setProperty ("plain", floatParameter->get());
    }
    else
    {
        object->setProperty ("kind", "continuous");
        object->setProperty ("plain", parameter.getValue());
    }

    return juce::var (object);
}

juce::var UTTWebView::buildHeroState() const
{
    auto* object = new juce::DynamicObject();
    auto* parameter = apvts.getParameter (lastTouchedParameterID);
    if (parameter == nullptr)
        parameter = apvts.getParameter ("gain");

    if (parameter != nullptr)
    {
        auto description = describeParameter (*parameter);
        auto* detail = description.getDynamicObject();
        const auto id = detail->getProperty ("id").toString();
        object->setProperty ("id", id);
        object->setProperty ("label", formatParameterLabel (id));
        object->setProperty ("text", detail->getProperty ("text"));
        object->setProperty ("unit", detail->getProperty ("unit"));
    }

    return juce::var (object);
}

juce::var UTTWebView::buildFullState() const
{
    auto* root = new juce::DynamicObject();
    juce::Array<juce::var> parameters;

    for (auto* parameter : processor.getParameters())
        if (dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter) != nullptr)
            parameters.add (describeParameter (*parameter));

    root->setProperty ("params", parameters);
    root->setProperty ("hero", buildHeroState());
    root->setProperty ("version", "v0.3.0");

    auto* engine = new juce::DynamicObject();
    engine->setProperty ("sampleRate", processor.getSampleRate());
    engine->setProperty ("blockSize", processor.getBlockSize());
    root->setProperty ("engine", juce::var (engine));

    auto* record = new juce::DynamicObject();
    record->setProperty ("desired", processor.getRecorder().isDesired());
    record->setProperty ("active", processor.getRecorder().isActive());
    record->setProperty ("path", processor.getRecordPath());
    record->setProperty ("dropped", (juce::int64) processor.getRecorder().getDroppedBlocks());
    record->setProperty ("error", false);
    root->setProperty ("record", juce::var (record));

    return juce::var (root);
}

juce::String UTTWebView::formatParameterValue (const juce::AudioProcessorParameter& parameter)
{
    auto* withID = dynamic_cast<const juce::AudioProcessorParameterWithID*> (&parameter);
    if (withID == nullptr)
        return parameter.getCurrentValueAsText();

    if (auto* boolParameter = dynamic_cast<const juce::AudioParameterBool*> (&parameter))
        return boolParameter->get() ? "ON" : "OFF";

    if (auto* choiceParameter = dynamic_cast<const juce::AudioParameterChoice*> (&parameter))
        return choiceParameter->getCurrentChoiceName();

    if (auto* floatParameter = dynamic_cast<const juce::AudioParameterFloat*> (&parameter))
    {
        const auto value = (double) floatParameter->get();
        const auto& id = withID->paramID;
        if (id == "phaseOffset")
            return juce::String (juce::roundToInt (value)) + juce::String::fromUTF8 ("\xc2\xb0");
        if (id == "stereoSeparation")
            return juce::String (juce::roundToInt (value)) + "%";
        if (id == "pan")
        {
            const auto rounded = juce::roundToInt (value);
            if (rounded == 0) return "C";
            return rounded < 0 ? "L" + juce::String (-rounded) : "R" + juce::String (rounded);
        }
        if (id == "gain" || id == "outputLGain" || id == "outputRGain")
            return (value >= 0.0 ? "+" : "") + juce::String (value, 1) + " dB";
        return juce::String (value, 2);
    }

    return parameter.getCurrentValueAsText();
}

juce::String UTTWebView::formatParameterLabel (const juce::String& parameterID)
{
    struct Entry { const char* id; const char* label; };
    static constexpr Entry entries[] {
        { "dcOffsetBypass", "DC BLOCK" },
        { "phaseInvertL", "PHASE L" },
        { "phaseInvertR", "PHASE R" },
        { "phaseOffset", "PHASE" },
        { "gain", "GAIN" },
        { "msMode", "M/S" },
        { "monoMode", "MONO" },
        { "stereoSeparation", "WIDTH" },
        { "monoToStereo", "M TO S" },
        { "channelSwap", "SWAP" },
        { "pan", "PAN" },
        { "outputLGain", "L GAIN" },
        { "outputRGain", "R GAIN" },
        { "outputLMute", "L MUTE" },
        { "outputRMute", "R MUTE" },
        { "outputLSolo", "L SOLO" },
        { "outputRSolo", "R SOLO" },
        { "recordEnable", "RECORD" }
    };

    for (const auto& entry : entries)
        if (parameterID == entry.id)
            return entry.label;

    return parameterID.toUpperCase();
}

void UTTWebView::sendFullState()
{
    emitEvent ("fullState", buildFullState());

    auto& recorder = processor.getRecorder();
    lastRecordOn = recorder.isDesired();
    lastRecordActive = recorder.isActive();
    lastRecordPath = processor.getRecordPath();
    lastDroppedSeen = recorder.getDroppedBlocks();
}

void UTTWebView::flushDirtyParameters()
{
    if (dirtyParameters.isEmpty())
        return;

    auto* root = new juce::DynamicObject();
    juce::Array<juce::var> parameters;
    for (const auto& id : dirtyParameters)
        if (auto* parameter = apvts.getParameter (id))
            parameters.add (describeParameter (*parameter));

    dirtyParameters.clearQuick();
    root->setProperty ("params", parameters);
    root->setProperty ("hero", buildHeroState());
    emitEvent ("parameterState", juce::var (root));
}

void UTTWebView::pushMeterFrame()
{
    const auto& metering = processor.getMetering();
    const auto& phase = processor.getPhaseMeter();
    const auto& lufs = processor.getLUFSMeter();

    auto* object = new juce::DynamicObject();
    object->setProperty ("peakL", metering.getLPeak());
    object->setProperty ("peakR", metering.getRPeak());
    object->setProperty ("rmsL", metering.getLRMS());
    object->setProperty ("rmsR", metering.getRRMS());
    object->setProperty ("correlation", phase.getCorrelation());
    object->setProperty ("momentary", lufs.getMomentary());
    object->setProperty ("shortTerm", lufs.getShortTerm());
    object->setProperty ("integrated", lufs.getIntegrated());
    emitEvent ("meterFrame", juce::var (object));
}

void UTTWebView::pushScopeFrame()
{
    const auto generation = processor.getOscilloscope().copyLatestFrame (scopeL, scopeR);
    if (generation == 0 || generation == lastScopeGeneration)
        return;

    lastScopeGeneration = generation;
    const auto pointCount = juce::jmin ((int) scopeL.size(), (int) scopeR.size());
    if (pointCount < 2)
        return;

    juce::Array<juce::var> left;
    juce::Array<juce::var> right;
    left.ensureStorageAllocated (scopePoints);
    right.ensureStorageAllocated (scopePoints);

    const auto stride = (double) (pointCount - 1) / (scopePoints - 1);
    for (int index = 0; index < scopePoints; ++index)
    {
        const auto sourceIndex = juce::roundToInt (index * stride);
        left.add (scopeL[(size_t) sourceIndex]);
        right.add (scopeR[(size_t) sourceIndex]);
    }

    auto* object = new juce::DynamicObject();
    object->setProperty ("generation", (juce::int64) generation);
    object->setProperty ("left", left);
    object->setProperty ("right", right);
    emitEvent ("scopeFrame", juce::var (object));
}

void UTTWebView::pushRecordState()
{
    auto& recorder = processor.getRecorder();
    const auto on = recorder.isDesired();
    const auto active = recorder.isActive();
    const auto dropped = recorder.getDroppedBlocks();
    const auto path = processor.getRecordPath();

    auto* object = new juce::DynamicObject();
    object->setProperty ("desired", on);
    object->setProperty ("active", active);
    object->setProperty ("path", path);
    object->setProperty ("dropped", (juce::int64) dropped);
    object->setProperty ("error", recordErrorPending);
    recordErrorPending = false;

    lastRecordOn = on;
    lastRecordActive = active;
    lastDroppedSeen = dropped;
    lastRecordPath = path;
    emitEvent ("recordState", juce::var (object));
}

void UTTWebView::emitEvent (const juce::Identifier& eventID, juce::var payload)
{
    if (frontendReady && browser != nullptr)
        browser->emitEventIfBrowserIsVisible (eventID, std::move (payload));
}

void UTTWebView::endActiveParameterGestures()
{
    for (auto* parameter : activeParameterGestures)
        if (parameter != nullptr)
            parameter->endChangeGesture();
    activeParameterGestures.clearQuick();
}

void UTTWebView::setSurfaceActive (bool active)
{
    if (surfaceActive == active)
        return;

    surfaceActive = active;
    if (! active)
    {
        endActiveParameterGestures();
        if (frontendReady)
        {
            auto* visibility = new juce::DynamicObject();
            visibility->setProperty ("active", false);
            emitEvent ("surfaceVisibility", juce::var (visibility));
        }
        stopTimer();
        return;
    }

    if (interfaceRequested)
        startTimerHz (timerHz);

    if (frontendReady)
    {
        sendFullState();
        auto* visibility = new juce::DynamicObject();
        visibility->setProperty ("active", true);
        emitEvent ("surfaceVisibility", juce::var (visibility));
    }
}

void UTTWebView::pageFinishedLoading (const juce::String& url)
{
    juce::ignoreUnused (url);
}

void UTTWebView::pageLoadFailed (const juce::String& error)
{
    reportFailure (SurfaceState::loadError, "Web UI load failed: " + error);
}

void UTTWebView::reportFailure (SurfaceState state, const juce::String& reason)
{
    if (failureReported)
        return;

    failureReported = true;
    frontendReady = false;
    surfaceState = state;
    stopTimer();
    endActiveParameterGestures();

    if (browser != nullptr)
        browser->setVisible (false);

    auto callback = failureHandler;
    if (callback)
        juce::MessageManager::callAsync ([callback = std::move (callback), state, reason]
        {
            callback (state, reason);
        });
}

#pragma once

#include <JuceHeader.h>

#include <functional>
#include <vector>

class UTTProcessor;

// Owns UTT's embedded, offline WebView surface and its versioned native bridge.
// The processor/APVTS remain the single source of truth for every parameter and
// all telemetry. If the platform web view is unavailable or the document never
// becomes ready, the owner receives a failure callback and presents the native fallback.
class UTTWebView final : public juce::Component,
                         private juce::Timer,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::WebViewLifetimeListener
{
public:
    enum class SurfaceState
    {
        loading,
        ready,
        unavailable,
        loadError
    };

    using FailureHandler = std::function<void (SurfaceState, juce::String)>;
    using ReadyHandler = std::function<void()>;

    UTTWebView (UTTProcessor&, juce::AudioProcessorEditor&, FailureHandler, ReadyHandler);
    ~UTTWebView() override;

    void loadInterface();
    bool isReady() const noexcept { return surfaceState == SurfaceState::ready; }

    void resized() override;
    void visibilityChanged() override;

private:
    class Browser;
    using Resource = juce::WebBrowserComponent::Resource;
    using ResourceResult = std::optional<Resource>;

    static juce::WebBrowserComponent::Options makeBrowserOptions (UTTWebView&);
    static ResourceResult getResource (const juce::String& url);
    static juce::String getMimeType (const juce::String& path);

    void webViewConstructed (juce::WebBrowserComponent*) override;
    void webViewDestructed (juce::WebBrowserComponent*) override;

    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void handleUiReady();
    void handleParameterGesture (const juce::var& payload);
    void handleParameterReset (const juce::var& payload);
    void handleParameterContext (const juce::var& payload);
    void handleRecordBrowse();

    juce::var buildFullState() const;
    juce::var describeParameter (const juce::AudioProcessorParameter&) const;
    juce::var buildHeroState() const;
    static juce::String formatParameterValue (const juce::AudioProcessorParameter&);
    static juce::String formatParameterLabel (const juce::String& parameterID);

    void sendFullState();
    void flushDirtyParameters();
    void pushMeterFrame();
    void pushScopeFrame();
    void pushRecordState();
    void emitEvent (const juce::Identifier&, juce::var payload);

    void endActiveParameterGestures();
    void setSurfaceActive (bool active);
    void pageFinishedLoading (const juce::String& url);
    void pageLoadFailed (const juce::String& error);
    void reportFailure (SurfaceState, const juce::String& reason);

    UTTProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::AudioProcessorEditor& editor;
    FailureHandler failureHandler;
    ReadyHandler readyHandler;

    std::unique_ptr<Browser> browser;
    juce::WebBrowserComponent* constructedBrowser = nullptr;

    std::vector<float> scopeL;
    std::vector<float> scopeR;
    std::uint64_t lastScopeGeneration = 0;
    std::uint64_t lastDroppedSeen = 0;
    int tick = 0;

    bool interfaceRequested = false;
    bool frontendReady = false;
    bool surfaceActive = true;
    bool failureReported = false;
    bool lastRecordOn = false;
    bool lastRecordActive = false;
    bool recordErrorPending = false;
    juce::uint32 readyDeadlineMs = 0;
    SurfaceState surfaceState = SurfaceState::loading;

    juce::String lastRecordPath;
    juce::String lastTouchedParameterID { "gain" };
    juce::StringArray dirtyParameters;
    juce::StringArray listenedParameters;
    juce::Array<juce::AudioProcessorParameter*> activeParameterGestures;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UTTWebView)
};

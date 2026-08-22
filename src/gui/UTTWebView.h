#pragma once
#include <JuceHeader.h>
#include <vector>

class UTTProcessor;

// WebView frontend host + native bridge (contract v1).
//
// JS -> native (registered as native functions, all called on the message
// thread): uttUiReady, uttGesture{id,phase,value}, uttResetParam{id},
// uttRecordBrowse, uttAbout, uttParamMenu{id}.
//
// native -> JS (via emitEventIfBrowserIsVisible):
//   uttParamState {params:{id:{v,t}}}  - dirty subset, flushed ~30 Hz
//   uttMeter      {lp,rp,lr,rr,corr,lm,ls,li} - ~25 Hz from atomics
//   uttScope      {g,l[],r[]}          - ~12 Hz, decimated triple-buffer frame
//   uttRecord     {on,active,path,error,dropped} - on change
class UTTWebView : public juce::WebBrowserComponent,
                   private juce::Timer,
                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    UTTWebView (UTTProcessor&, juce::AudioProcessorEditor& owner);
    ~UTTWebView() override;

    // must be called after construction, once the component is on screen
    void loadInterface();

    void setBridgeActive (bool active);
    bool isBridgeActive() const { return bridgeActive; }

private:
    // Timer: transport (meters 25 Hz / scope 12 Hz / dirty params / record)
    void timerCallback() override;

    // APVTS::Listener (message thread): mark parameter dirty
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // bridge handlers
    void handleGesture (const juce::var& payload, const juce::WebBrowserComponent::NativeFunctionCompletion&);
    void handleResetParam (const juce::var& payload);
    void handleRecordBrowse();
    void handleAbout();
    void handleParamMenu (const juce::var& payload);

    juce::var buildSnapshot();
    void flushDirtyParams();
    void pushRecordState();
    void pushMeterFrame();
    void pushScopeFrame();

    static juce::String formatParameterValue (const juce::AudioProcessorParameter& param);

    UTTProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::AudioProcessorEditor& editor;

    std::vector<float> scopeL, scopeR;
    std::uint64_t lastScopeGeneration = 0;
    int tick = 0;

    bool bridgeActive = false;
    bool recordErrorPending = false;
    std::uint64_t lastDroppedSeen = 0;
    bool lastRecordOn = false, lastRecordActive = false;
    juce::String lastRecordPath;

    // dirty parameter ids pending flush to the UI
    juce::StringArray dirtyParams;
    juce::StringArray listenedParams;
    juce::Array<juce::Identifier> eventNames;

    JUCE_DECLARE_NON_COPYABLE (UTTWebView)
};

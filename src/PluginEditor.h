#pragma once
#include <JuceHeader.h>
#include <memory>

class UTTProcessor;
class UTTWebView;
class FallbackPanel;

class UTTEditor : public juce::AudioProcessorEditor
{
public:
    explicit UTTEditor (UTTProcessor&);
    ~UTTEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuild();
    void showWebView();
    void showFallback (const juce::String& surfaceState, const juce::String& reason);

    UTTProcessor& processor;

    std::unique_ptr<UTTWebView> webView;
    std::unique_ptr<FallbackPanel> fallback;
    std::size_t webUIGeneration = 0;

    JUCE_DECLARE_NON_COPYABLE (UTTEditor)
};

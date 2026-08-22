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

    UTTProcessor& processor;

    std::unique_ptr<UTTWebView> webView;
    std::unique_ptr<FallbackPanel> fallback;

    JUCE_DECLARE_NON_COPYABLE (UTTEditor)
};

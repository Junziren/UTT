#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>

class UTTProcessor;

// Functional native fallback for WebView2/runtime/load failures. It keeps all
// host parameters available rather than leaving an editor-sized blank surface.
class FallbackPanel : public juce::Component
{
public:
    using RetryHandler = std::function<void()>;

    FallbackPanel (UTTProcessor&, const juce::String& surfaceState,
                   const juce::String& detail, RetryHandler);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void addRow (const juce::String& paramID, const juce::String& label);

    UTTProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label titleLabel;
    juce::Label noticeLabel;
    juce::TextButton retryButton { "RETRY WEB UI" };
    bool retryVisible = false;
    struct Row
    {
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttach;
    };
    std::vector<Row> rows;

    JUCE_DECLARE_NON_COPYABLE (FallbackPanel)
};

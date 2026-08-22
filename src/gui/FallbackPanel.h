#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

class UTTProcessor;

// Minimal native fallback when the WebView2 runtime is unavailable.
// Plain juce controls + APVTS attachments; no theme system, no meters.
// Audio functionality is completely unaffected by this panel.
class FallbackPanel : public juce::Component
{
public:
    explicit FallbackPanel (UTTProcessor&);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void addRow (const juce::String& paramID, const juce::String& label);

    UTTProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label noticeLabel;
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

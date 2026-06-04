#pragma once
#include <JuceHeader.h>
#include "gui/theme/UTTLookAndFeel.h"
#include "gui/theme/ThemeLoader.h"
#include "gui/controls/Controls.h"
#include "gui/AboutComponent.h"

class UTTProcessor;

class UTTEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit UTTEditor(UTTProcessor& p);
    ~UTTEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    UTTProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    UTTLookAndFeel uttLookAndFeel;

    // Header
    juce::Label titleLabel;
    juce::ComboBox themeCombo;
    juce::TextButton aboutBtn;
    AboutComponent aboutOverlay;

    // Input section
    juce::GroupComponent inputGroup;
    LedButton dcOffsetBtn;
    LedButton phaseInvL, phaseInvR;
    VUKnob phaseOffsetKnob;

    // Processing section
    juce::GroupComponent procGroup;
    VUKnob gainKnob;
    juce::ComboBox msModeCombo;
    juce::ComboBox monoModeCombo;
    VUKnob separationKnob;
    juce::ComboBox monoToStereoCombo;
    LedButton channelSwapBtn;
    VUKnob panKnob;

    // Output section
    juce::GroupComponent outputGroup;
    VUKnob outLGainKnob, outRGainKnob;
    LedButton outLMuteBtn, outRMuteBtn, outLSoloBtn, outRSoloBtn;

    // Metering
    juce::GroupComponent meterGroup;
    VUMeterBar meterL, meterR;
    UTTScope scope;
    PhaseCorrelationBar phaseBar;

    // Recording
    juce::GroupComponent recordGroup;
    LedButton recordEnableBtn;
    juce::TextButton browseBtn;
    juce::Label pathLabel;

    // APVTS attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    void addSliderAttach(const juce::String& paramID, juce::Slider& slider);
    void addComboAttach(const juce::String& paramID, juce::ComboBox& combo);

    // Theme switching
    void applyTheme(int index);
    void applyThemeToAll(const UTTTheme& theme);

    // Control lists for theme propagation
    std::vector<VUKnob*> knobs;
    std::vector<LedButton*> leds;
    std::vector<VUMeterBar*> meters;
    std::vector<PhaseCorrelationBar*> phaseBars;
    std::vector<UTTScope*> scopes;

    JUCE_DECLARE_NON_COPYABLE(UTTEditor)
};


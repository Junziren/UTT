#include "PluginProcessor.h"
#include "FallbackPanel.h"

namespace
{
const auto paper = juce::Colour (0xfffafaf8);
const auto ink = juce::Colour (0xff0a0a0a);
const auto grey = juce::Colour (0xffd4d4d2);
const auto accent = juce::Colour (0xff002fa7);
} // namespace

FallbackPanel::FallbackPanel (UTTProcessor& p,
                              const juce::String& surfaceState,
                              const juce::String& detail,
                              RetryHandler retryHandler)
    : processor (p), apvts (p.apvts)
{
    retryVisible = surfaceState != "LOADING";

    titleLabel.setText (retryVisible ? "UTT  /  NATIVE FALLBACK"
                                     : "UTT  /  NATIVE LOADING",
                        juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, ink);
    addAndMakeVisible (titleLabel);

    noticeLabel.setText ("WEB UI " + surfaceState + "  |  " + detail,
                         juce::dontSendNotification);
    noticeLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    noticeLabel.setColour (juce::Label::textColourId, ink.withAlpha (0.76f));
    noticeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (noticeLabel);

    retryButton.setColour (juce::TextButton::buttonColourId, accent);
    retryButton.setColour (juce::TextButton::textColourOffId, paper);
    retryButton.setColour (juce::TextButton::buttonOnColourId, ink);
    retryButton.setTooltip ("Retry the embedded WebView UI");
    retryButton.onClick = std::move (retryHandler);
    if (retryVisible)
        addAndMakeVisible (retryButton);

    struct Spec { const char* id; const char* label; };
    static const Spec specs[] = {
        { "phaseOffset",      "PHASE OFFSET" },
        { "gain",             "GAIN" },
        { "stereoSeparation", "STEREO WIDTH" },
        { "pan",              "PAN" },
        { "outputLGain",      "L GAIN" },
        { "outputRGain",      "R GAIN" },
        { "msMode",           "M/S MODE" },
        { "monoMode",         "MONO MODE" },
        { "monoToStereo",     "M TO S" },
        { "dcOffsetBypass",   "DC BLOCK" },
        { "phaseInvertL",     "PHASE L" },
        { "phaseInvertR",     "PHASE R" },
        { "channelSwap",      "CHANNEL SWAP" },
        { "outputLMute",      "L MUTE" },
        { "outputRMute",      "R MUTE" },
        { "outputLSolo",      "L SOLO" },
        { "outputRSolo",      "R SOLO" },
        { "recordEnable",     "RECORD" },
    };

    for (const auto& spec : specs)
        addRow (spec.id, spec.label);
}

void FallbackPanel::addRow (const juce::String& parameterID, const juce::String& label)
{
    Row row;
    row.label = std::make_unique<juce::Label> (parameterID, label);
    row.label->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    row.label->setColour (juce::Label::textColourId, ink);
    addAndMakeVisible (*row.label);

    if (auto* parameter = apvts.getParameter (parameterID))
    {
        if (dynamic_cast<juce::AudioParameterFloat*> (parameter) != nullptr)
        {
            row.slider = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal,
                                                          juce::Slider::TextBoxRight);
            row.slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 78, 22);
            row.slider->setColour (juce::Slider::backgroundColourId, grey);
            row.slider->setColour (juce::Slider::trackColourId, accent);
            row.slider->setColour (juce::Slider::thumbColourId, ink);
            row.slider->setColour (juce::Slider::textBoxOutlineColourId, ink);
            addAndMakeVisible (*row.slider);
            row.sliderAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, parameterID, *row.slider);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (parameter) != nullptr)
        {
            row.toggle = std::make_unique<juce::ToggleButton> ("ON");
            row.toggle->setColour (juce::ToggleButton::tickColourId, accent);
            row.toggle->setColour (juce::ToggleButton::textColourId, ink);
            addAndMakeVisible (*row.toggle);
            row.buttonAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, parameterID, *row.toggle);
        }
        else if (dynamic_cast<juce::AudioParameterChoice*> (parameter) != nullptr)
        {
            row.combo = std::make_unique<juce::ComboBox>();
            row.combo->setColour (juce::ComboBox::backgroundColourId, paper);
            row.combo->setColour (juce::ComboBox::outlineColourId, ink);
            row.combo->setColour (juce::ComboBox::textColourId, ink);
            addAndMakeVisible (*row.combo);
            row.comboAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, parameterID, *row.combo);
        }
    }

    rows.push_back (std::move (row));
}

void FallbackPanel::paint (juce::Graphics& graphics)
{
    graphics.fillAll (paper);
    graphics.setColour (ink);
    graphics.drawRect (getLocalBounds(), 1);
    graphics.setColour (grey);
    graphics.drawLine (0.0f, 42.0f, (float) getWidth(), 42.0f, 1.0f);
    graphics.setColour (accent);
    graphics.fillRect (12, 15, 8, 8);
}

void FallbackPanel::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    auto header = bounds.removeFromTop (30);
    if (retryVisible)
        retryButton.setBounds (header.removeFromRight (126).reduced (0, 3));
    titleLabel.setBounds (header);

    noticeLabel.setBounds (bounds.removeFromTop (34));
    bounds.removeFromTop (6);

    const auto rowsPerColumn = ((int) rows.size() + 1) / 2;
    const auto columnGap = 18;
    const auto columnWidth = (bounds.getWidth() - columnGap) / 2;
    const auto rowHeight = juce::jmax (28, bounds.getHeight() / rowsPerColumn);

    for (int index = 0; index < (int) rows.size(); ++index)
    {
        const auto column = index / rowsPerColumn;
        const auto rowIndex = index % rowsPerColumn;
        auto rowBounds = juce::Rectangle<int> (bounds.getX() + column * (columnWidth + columnGap),
                                               bounds.getY() + rowIndex * rowHeight,
                                               columnWidth, rowHeight).reduced (0, 2);
        auto& row = rows[(size_t) index];
        row.label->setBounds (rowBounds.removeFromLeft (118));
        if (row.slider != nullptr) row.slider->setBounds (rowBounds);
        if (row.toggle != nullptr) row.toggle->setBounds (rowBounds.removeFromLeft (70));
        if (row.combo != nullptr) row.combo->setBounds (rowBounds.removeFromLeft (150));
    }
}

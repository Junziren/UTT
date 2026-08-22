#include "PluginProcessor.h"
#include "FallbackPanel.h"

namespace
{
const char* const fallbackNotice =
    "未检测到 WebView2 运行时 — 使用简化控制面板（音频功能不受影响）。"
    "安装 Microsoft WebView2 Evergreen Runtime 后重新打开编辑器可获得完整界面。";
}

FallbackPanel::FallbackPanel (UTTProcessor& p)
    : processor (p), apvts (p.apvts)
{
    noticeLabel.setText (juce::String::fromUTF8 (fallbackNotice),
                         juce::dontSendNotification);
    noticeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffAC4B2C));
    noticeLabel.setFont (juce::Font (13.0f));
    noticeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (noticeLabel);

    struct Spec { const char* id; const char* label; };
    static const Spec specs[] = {
        { "phaseOffset",     "\xe7\x9b\xb8\xe4\xbd\x8d\xe5\x81\x8f\xe7\xa7\xbb" },   // 相位偏移
        { "gain",            "\xe5\xa2\x9e\xe7\x9b\x8a" },                          // 增益
        { "stereoSeparation", "\xe7\xab\x8b\xe4\xbd\x93\xe5\xa3\xb0\xe5\x88\x86\xe7\xa6\xbb" }, // 立体声分离
        { "pan",             "\xe5\xa3\xb0\xe5\x83\x8f" },                          // 声像
        { "outputLGain",     "L \xe5\xa2\x9e\xe7\x9b\x8a" },                        // L 增益
        { "outputRGain",     "R \xe5\xa2\x9e\xe7\x9b\x8a" },                        // R 增益
        { "msMode",          "M/S" },
        { "monoMode",        "\xe5\x8d\x95\xe5\xa3\xb0\xe9\x81\x93" },              // 单声道
        { "monoToStereo",    "M\xe2\x86\x92S" },
        { "dcOffsetBypass",  "DC \xe9\x98\xbb\xe9\x9a\x94" },                       // DC 阻隔
        { "phaseInvertL",    "\xe7\x9b\xb8\xe4\xbd\x8d\xe5\x8f\x8d\xe8\xbd\xac L" },// 相位反转 L
        { "phaseInvertR",    "\xe7\x9b\xb8\xe4\xbd\x8d\xe5\x8f\x8d\xe8\xbd\xac R" },// 相位反转 R
        { "channelSwap",     "\xe5\xa3\xb0\xe9\x81\x93\xe4\xba\xa4\xe6\x8d\xa2" },  // 声道交换
        { "outputLMute",     "L Mute" },
        { "outputRMute",     "R Mute" },
        { "outputLSolo",     "L Solo" },
        { "outputRSolo",     "R Solo" },
        { "recordEnable",    "\xe5\xbd\x95\xe5\x88\xb6" },                          // 录制
    };

    for (auto& s : specs)
        addRow (s.id, s.label);
}

void FallbackPanel::addRow (const juce::String& paramID, const juce::String& label)
{
    Row r;
    r.label = std::make_unique<juce::Label> (paramID, label);
    r.label->setFont (juce::Font (13.0f));
    addAndMakeVisible (*r.label);

    if (auto* param = apvts.getParameter (paramID))
    {
        if (dynamic_cast<juce::AudioParameterFloat*> (param) != nullptr)
        {
            r.slider = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            r.slider->setColour (juce::Slider::backgroundColourId, juce::Colour (0xffD7D3C5));
            r.slider->setColour (juce::Slider::trackColourId, juce::Colour (0xff1C1B14));
            r.slider->setColour (juce::Slider::thumbColourId, juce::Colour (0xff3D4A78));
            addAndMakeVisible (*r.slider);
            r.sliderAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, paramID, *r.slider);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            r.toggle = std::make_unique<juce::ToggleButton>();
            addAndMakeVisible (*r.toggle);
            r.buttonAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, paramID, *r.toggle);
        }
        else if (dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr)
        {
            r.combo = std::make_unique<juce::ComboBox>();
            addAndMakeVisible (*r.combo);
            r.comboAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, paramID, *r.combo);
        }
    }
    rows.push_back (std::move (r));
}

void FallbackPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffE8E5DB));
}

void FallbackPanel::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    noticeLabel.setBounds (bounds.removeFromTop (44));
    bounds.removeFromTop (8);

    const int half = (int) rows.size() / 2;
    const int leftW = bounds.getWidth() / 2 - 8;
    const int rightX = bounds.getX() + bounds.getWidth() / 2 + 8;

    for (int i = 0; i < (int) rows.size(); ++i)
    {
        const bool isLeft = i < half;
        auto area = isLeft
            ? juce::Rectangle<int> (bounds.getX(), bounds.getY() + i * 30, leftW, 28)
            : juce::Rectangle<int> (rightX, bounds.getY() + (i - half) * 30, leftW, 28);

        auto& r = rows[(size_t) i];
        r.label->setBounds (area.removeFromLeft (110));
        if (r.slider) r.slider->setBounds (area);
        if (r.toggle) r.toggle->setBounds (area.removeFromLeft (40));
        if (r.combo)  r.combo->setBounds (area.removeFromLeft (140));
    }
}

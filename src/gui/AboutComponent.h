#pragma once
#include <JuceHeader.h>

class AboutComponent : public juce::Component
{
public:
    AboutComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override { setVisible(false); }

private:
    juce::Label titleLabel;
    juce::Label devLabel;
    juce::HyperlinkButton bilibiliLink;
    juce::Label licenseLabel;
    juce::Label versionLabel;
    juce::Label closeLabel;

    JUCE_DECLARE_NON_COPYABLE(AboutComponent)
};
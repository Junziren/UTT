#include "Panels.h"
#include "PluginProcessor.h"

// Minimal ChannelStrip - just one ToggleButton
ChannelStrip::ChannelStrip(UTTProcessor& proc) : processor(proc)
{
    dcOffsetBtn = std::make_unique<juce::ToggleButton>("DC Offset");
    addAndMakeVisible(*dcOffsetBtn);
}

void ChannelStrip::attachParameters() {}

void ChannelStrip::resized()
{
    auto a = getLocalBounds().reduced(4, 4);
    dcOffsetBtn->setBounds(a.removeFromTop(24));
}

void ChannelStrip::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

// RecordingPanel stubs (not used, but must exist)
RecordingPanel::RecordingPanel(UTTProcessor&) {}
void RecordingPanel::resized() {}
void RecordingPanel::paint(juce::Graphics&) {}

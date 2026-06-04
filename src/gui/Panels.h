#pragma once
#include <JuceHeader.h>
#include <memory>

class UTTProcessor;

class ChannelStrip : public juce::Component
{
public:
    explicit ChannelStrip(UTTProcessor& proc);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    UTTProcessor& processor;
    std::unique_ptr<juce::ToggleButton> dcOffsetBtn;
    void attachParameters();
};

class RecordingPanel : public juce::Component
{
public:
    explicit RecordingPanel(UTTProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
};

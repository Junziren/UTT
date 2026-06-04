// Diagnostic version: RecordingPanel with only standard JUCE controls
#pragma once
#include <JuceHeader.h>
#include <memory>

class UTTProcessor;

class RecordingPanel : public juce::Component
{
public:
    explicit RecordingPanel(UTTProcessor& proc);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    UTTProcessor& processor;
    std::unique_ptr<juce::TextButton> recordBtn;
    std::unique_ptr<juce::Label> pathLabel;
    std::unique_ptr<juce::TextEditor> pathEditor;
    void browseForPath();
};

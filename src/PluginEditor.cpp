#include "util/CrashLog.h"
#include "util/TraceLog.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Parameters.h"

// ================================================================
// Helpers
// ================================================================
void UTTEditor::addSliderAttach(const juce::String& paramID, juce::Slider& slider)
{
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider));
}

void UTTEditor::addComboAttach(const juce::String& paramID, juce::ComboBox& combo)
{
    comboAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramID, combo));
}

// ================================================================
// Theme switching
// ================================================================
void UTTEditor::applyTheme(int index)
{
    auto lf = ThemeLoader::getBuiltIn(index);
    if (lf)
    {
        auto& t = lf->getTheme();
        uttLookAndFeel.setTheme(t); repaint();
    }
}

void UTTEditor::applyThemeToAll(const UTTTheme& theme)
{
    for (auto* k : knobs)      k->setTheme(&theme);
    for (auto* l : leds)       l->setTheme(&theme);
    for (auto* m : meters)     m->setTheme(&theme);
    for (auto* p : phaseBars)  p->setTheme(&theme);
    for (auto* s : scopes)     s->setTheme(&theme);
    titleLabel.setColour(juce::Label::textColourId, theme.textHighlight);
    repaint();
}

// ================================================================
// Constructor
// ================================================================
UTTEditor::UTTEditor(UTTProcessor& p) : AudioProcessorEditor(&p), processor(p), apvts(p.apvts),
    gainKnob("Gain", -96.0f, 24.0f, "dB"),
    phaseOffsetKnob("Phase", 0.0f, 360.0f, "deg"),
    separationKnob("Sep.", 0.0f, 100.0f, "%"),
    panKnob("Pan", -100.0f, 100.0f, "%"),
    outLGainKnob("L Gain", -96.0f, 12.0f, "dB"),
    outRGainKnob("R Gain", -96.0f, 12.0f, "dB"),
    dcOffsetBtn("DC Offset"),
    phaseInvL("Inv L"), phaseInvR("Inv R"),
    channelSwapBtn("Swap"),
    outLMuteBtn("Mute L"), outRMuteBtn("Mute R"),
    outLSoloBtn("Solo L"), outRSoloBtn("Solo R"),
    recordEnableBtn("Rec")
{
    UTT_TRACE();
    setLookAndFeel(&uttLookAndFeel);
    setSize(920, 660);

    auto& theme = uttLookAndFeel.getTheme();

    // Register controls for theme propagation
    knobs = { &gainKnob, &phaseOffsetKnob, &separationKnob, &panKnob, &outLGainKnob, &outRGainKnob };
    leds = { &dcOffsetBtn, &phaseInvL, &phaseInvR, &channelSwapBtn, &outLMuteBtn, &outRMuteBtn, &outLSoloBtn, &outRSoloBtn, &recordEnableBtn };
    meters = { &meterL, &meterR };
    phaseBars = { &phaseBar };
    scopes = { &scope };

    // ── Header ───────────────────────────────────────────
    titleLabel.setText("UTT Utility Toolbox", juce::dontSendNotification);
    titleLabel.setFont(uttLookAndFeel.getMainFont(theme.fontSizeTitle));
    titleLabel.setColour(juce::Label::textColourId, theme.textHighlight);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    themeCombo.addItem("Channel Strip", 1);
    themeCombo.addItem("Dark Modern", 2);
    themeCombo.addItem("Retro Light", 3);
    themeCombo.onChange = [this]()
    {
        int idx = themeCombo.getSelectedItemIndex();
        applyTheme(idx);
    };
    addAndMakeVisible(themeCombo);

    aboutBtn.setButtonText("?");
    aboutBtn.setTooltip("About UTT");
    addAndMakeVisible(aboutBtn);
    aboutBtn.onClick = [this]() { aboutOverlay.setVisible(!aboutOverlay.isVisible()); if (aboutOverlay.isVisible()) aboutOverlay.toFront(false); };

    addAndMakeVisible(aboutOverlay);
    aboutOverlay.setVisible(false);
    addComboAttach("themeIndex", themeCombo);

    // ── Input ────────────────────────────────────────────
    inputGroup.setText("INPUT");
    addAndMakeVisible(inputGroup);
    dcOffsetBtn.setTheme(&theme); addAndMakeVisible(dcOffsetBtn);
    phaseInvL.setTheme(&theme);   addAndMakeVisible(phaseInvL);
    phaseInvR.setTheme(&theme);   addAndMakeVisible(phaseInvR);
    phaseOffsetKnob.setTheme(&theme); addAndMakeVisible(phaseOffsetKnob);

    dcOffsetBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("dcOffsetBypass")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    phaseInvL.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("phaseInvertL")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    phaseInvR.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("phaseInvertR")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    addSliderAttach("phaseOffset", phaseOffsetKnob.slider);

    // ── Processing ───────────────────────────────────────
    procGroup.setText("PROCESSING");
    addAndMakeVisible(procGroup);
    gainKnob.setTheme(&theme); addAndMakeVisible(gainKnob);
    msModeCombo.addItem("Bypass", 1); msModeCombo.addItem("Encode", 2); msModeCombo.addItem("Decode", 3);
    addAndMakeVisible(msModeCombo);
    monoModeCombo.addItem("Off", 1); monoModeCombo.addItem("L+R Sum", 2); monoModeCombo.addItem("Mid Only", 3); monoModeCombo.addItem("Side Only", 4);
    addAndMakeVisible(monoModeCombo);
    separationKnob.setTheme(&theme); addAndMakeVisible(separationKnob);
    monoToStereoCombo.addItem("Off", 1); monoToStereoCombo.addItem("L->Both", 2); monoToStereoCombo.addItem("R->Both", 3);
    addAndMakeVisible(monoToStereoCombo);
    channelSwapBtn.setTheme(&theme); addAndMakeVisible(channelSwapBtn);
    panKnob.setTheme(&theme); addAndMakeVisible(panKnob);

    addSliderAttach("gain", gainKnob.slider);
    addComboAttach("msMode", msModeCombo);
    addComboAttach("monoMode", monoModeCombo);
    addSliderAttach("stereoSeparation", separationKnob.slider);
    addComboAttach("monoToStereo", monoToStereoCombo);
    channelSwapBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("channelSwap")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    addSliderAttach("pan", panKnob.slider);

    // ── Output ───────────────────────────────────────────
    outputGroup.setText("OUTPUT");
    addAndMakeVisible(outputGroup);
    outLGainKnob.setTheme(&theme); addAndMakeVisible(outLGainKnob);
    outRGainKnob.setTheme(&theme); addAndMakeVisible(outRGainKnob);
    outLMuteBtn.setTheme(&theme);  addAndMakeVisible(outLMuteBtn);
    outRMuteBtn.setTheme(&theme);  addAndMakeVisible(outRMuteBtn);
    outLSoloBtn.setTheme(&theme);  addAndMakeVisible(outLSoloBtn);
    outRSoloBtn.setTheme(&theme);  addAndMakeVisible(outRSoloBtn);

    addSliderAttach("outputLGain", outLGainKnob.slider);
    addSliderAttach("outputRGain", outRGainKnob.slider);
    outLMuteBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("outputLMute")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    outRMuteBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("outputRMute")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    outLSoloBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("outputLSolo")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    outRSoloBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("outputRSolo")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    // ── Metering ─────────────────────────────────────────
    meterGroup.setText("METERING");
    addAndMakeVisible(meterGroup);
    meterL.isLeft = true;  meterL.setTheme(&theme); addAndMakeVisible(meterL);
    meterR.isLeft = false; meterR.setTheme(&theme); addAndMakeVisible(meterR);
    scope.setTheme(&theme); addAndMakeVisible(scope);
    phaseBar.setTheme(&theme); addAndMakeVisible(phaseBar);

    // ── Recording ────────────────────────────────────────
    recordGroup.setText("RECORDING");
    addAndMakeVisible(recordGroup);
    recordEnableBtn.setTheme(&theme); addAndMakeVisible(recordEnableBtn);
    recordEnableBtn.onChange = [this](bool on) {
        if (auto* p = apvts.getParameter("recordEnable")) p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };


    browseBtn.setButtonText("Browse...");
    addAndMakeVisible(browseBtn);
    browseBtn.onClick = [this]() {
        UTT_LOG("Browse button clicked");
        auto fc = std::make_shared<juce::FileChooser>(
            "Save recording as...",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav");
        fc->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, fc](const juce::FileChooser& chooser) {
                auto result = chooser.getResult();
                if (result != juce::File()) {
                    if (result.getFileExtension().isEmpty())
                        result = result.withFileExtension(".wav");
                    processor.setRecordPath(result.getFullPathName());
                    pathLabel.setText(result.getFileName(), juce::dontSendNotification);
                }
            });
    };

    pathLabel.setText("", juce::dontSendNotification);
    pathLabel.setFont(11.0f);
    pathLabel.setColour(juce::Label::textColourId, theme.textSecondary);
    addAndMakeVisible(pathLabel);

    startTimerHz(20);
}

// ================================================================
// Destructor
// ================================================================
UTTEditor::~UTTEditor()
{
    UTT_TRACE();
    setLookAndFeel(nullptr);
    stopTimer();
}

// ================================================================
// Layout
// ================================================================
void UTTEditor::resized()
{
    UTT_TRACE();
    auto a = getLocalBounds().reduced(8);
    auto w = a.getWidth();

    // Header
    titleLabel.setBounds(a.removeFromTop(24));

    // Theme combo (right-aligned, on header line)
    auto hdrLine = juce::Rectangle<int>(a.getX(), 4, a.getRight() - 4, 22);
    aboutBtn.setBounds(hdrLine.removeFromRight(24));
    hdrLine.removeFromRight(4);
    aboutBtn.setBounds(hdrLine.removeFromRight(24));
    hdrLine.removeFromRight(4);
    themeCombo.setBounds(hdrLine.removeFromRight(130));

    a.removeFromTop(4);

    // Three-column processing strip
    float thirdW = (w - 16) / 3.0f;
    auto stripArea = a.removeFromTop(170);

    auto col1 = stripArea.removeFromLeft((int)thirdW);
    stripArea.removeFromLeft(4);
    auto col2 = stripArea.removeFromLeft((int)thirdW);
    stripArea.removeFromLeft(4);
    auto col3 = stripArea;

    // Column 1: Input
    inputGroup.setBounds(col1);
    auto in = col1.reduced(8).withTrimmedTop(18);
    dcOffsetBtn.setBounds(in.removeFromTop(22));
    phaseInvL.setBounds(in.removeFromLeft(in.getWidth() / 2).removeFromTop(22));
    phaseInvR.setBounds(in.removeFromTop(22));
    in.removeFromTop(8);
    phaseOffsetKnob.setBounds(in.removeFromLeft(70).withHeight(80));

    // Column 2: Processing
    procGroup.setBounds(col2);
    auto pr = col2.reduced(8).withTrimmedTop(18);
    gainKnob.setBounds(pr.removeFromLeft(65).withHeight(80));
    msModeCombo.setBounds(pr.removeFromTop(22));
    monoModeCombo.setBounds(pr.removeFromTop(22));
    separationKnob.setBounds(pr.removeFromLeft(65).withHeight(80));
    monoToStereoCombo.setBounds(pr.removeFromTop(22));
    channelSwapBtn.setBounds(pr.removeFromTop(22));
    panKnob.setBounds(pr.removeFromLeft(65).withHeight(80));

    // Column 3: Output
    outputGroup.setBounds(col3);
    auto out = col3.reduced(8).withTrimmedTop(18);
    outLGainKnob.setBounds(out.removeFromLeft(65).withHeight(80));
    outRGainKnob.setBounds(out.removeFromLeft(65).withHeight(80));
    outLMuteBtn.setBounds(out.removeFromTop(22));
    outRMuteBtn.setBounds(out.removeFromTop(22));
    outLSoloBtn.setBounds(out.removeFromTop(22));
    outRSoloBtn.setBounds(out.removeFromTop(22));

    a.removeFromTop(6);

    // Metering
    auto meterArea = a.removeFromTop(a.getHeight() - 52);
    meterGroup.setBounds(meterArea);
    auto m = meterArea.reduced(8).withTrimmedTop(18);
    meterL.setBounds(m.removeFromLeft(24));
    meterR.setBounds(m.removeFromLeft(24));
    m.removeFromLeft(8);
    phaseBar.setBounds(m.removeFromBottom(28));
    scope.setBounds(m);

    // Recording
    recordGroup.setBounds(a);
    auto rec = a.reduced(8).withTrimmedTop(18);
    recordEnableBtn.setBounds(rec.removeFromLeft(40));
    rec.removeFromLeft(4);
    browseBtn.setBounds(rec.removeFromLeft(70));
    rec.removeFromLeft(4);
    pathLabel.setBounds(rec);

    aboutOverlay.setBounds(getLocalBounds());
}

// ================================================================
// Timer - update meters and sync button states
// ================================================================
void UTTEditor::timerCallback()
{
    UTT_TRACE();
    // Sync LedButton states with parameters
    if (auto* p = apvts.getRawParameterValue("dcOffsetBypass")) dcOffsetBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("phaseInvertL"))   phaseInvL.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("phaseInvertR"))   phaseInvR.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("channelSwap"))    channelSwapBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("outputLMute"))    outLMuteBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("outputRMute"))    outRMuteBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("outputLSolo"))    outLSoloBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("outputRSolo"))    outRSoloBtn.setState(p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue("recordEnable"))   recordEnableBtn.setState(p->load() > 0.5f);

    // Metering
    auto& m = processor.getMetering();
    meterL.setLevel(m.getLPeak(), m.getLRMS());
    meterR.setLevel(m.getRPeak(), m.getRRMS());

    // Scope
    auto& srcPts = processor.getOscilloscope().getPoints();
    std::vector<juce::Point<float>> jpts;
    jpts.reserve(srcPts.size());
    for (auto& p : srcPts) jpts.push_back({p.x, p.y});
    scope.setPoints(jpts);

    // Phase correlation
    phaseBar.setCorrelation(processor.getPhaseMeter().getCorrelation());
}

// ================================================================
// Paint
// ================================================================
void UTTEditor::paint(juce::Graphics& g)
{
    g.fillAll(uttLookAndFeel.getTheme().background);
}




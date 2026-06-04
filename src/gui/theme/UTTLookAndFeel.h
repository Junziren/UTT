#pragma once
#include <JuceHeader.h>

// ============================================================
// UTTTheme — Serum‑inspired theme: each skin is a folder with
// theme.json + optional Fonts/ + preview.png + colormap.json
// ============================================================
struct UTTTheme
{
    juce::String name             { "Channel Strip" };
    juce::String description      { "Classic console channel strip" };

    // ── Backgrounds ──────────────────────────────────────────
    juce::Colour background       { 0xff1a1a1a };
    juce::Colour panelBg          { 0xff2d2d2d };
    juce::Colour headerBg         { 0xff252525 };
    juce::Colour footerBg         { 0xff252525 };

    // ── Text ─────────────────────────────────────────────────
    juce::Colour textPrimary      { 0xffe8e8e8 };
    juce::Colour textSecondary    { 0xff999999 };
    juce::Colour textHighlight    { 0xffcc6600 };

    // ── Accents ──────────────────────────────────────────────
    juce::Colour accent           { 0xffcc6600 };
    juce::Colour accent2          { 0xffcc3300 };

    // ── Knob ─────────────────────────────────────────────────
    juce::Colour knobCap          { 0xffc8c8c8 };
    juce::Colour knobBase         { 0xff4a4a4a };
    juce::Colour knobIndicator    { 0xffcc6600 };
    juce::Colour knobArcBg        { 0xff3a3a3a };
    juce::Colour knobValueText    { 0xff00cc66 };

    // ── VU / Meters ──────────────────────────────────────────
    juce::Colour vuNeedle         { 0xffff4400 };
    juce::Colour vuBacklight      { 0xffffcc00 };
    juce::Colour meterBg          { 0xff0a0a0a };
    juce::Colour meterLow         { 0xff00cc66 };
    juce::Colour meterMid         { 0xffcccc00 };
    juce::Colour meterHigh        { 0xffcc0000 };
    juce::Colour meterPeakHold    { 0xffffffff };
    juce::Colour meterTick        { 0xff888888 };

    // ── Buttons / LEDs ───────────────────────────────────────
    juce::Colour buttonOn         { 0xffff3300 };
    juce::Colour buttonOff        { 0xff3d1a1a };
    juce::Colour buttonHover      { 0xffff5533 };
    juce::Colour buttonGlow       { 0x44ff3300 };

    // ── Borders / Separators ─────────────────────────────────
    juce::Colour border           { 0xff555555 };
    juce::Colour sectionLine      { 0xff333333 };

    // ── Scope / Visualizer ───────────────────────────────────
    juce::Colour scopeBg          { 0xff0d0d0d };
    juce::Colour scopeGrid        { 0xff1a1a1a };
    juce::Colour scopeLine        { 0xff00ff66 };
    juce::Colour scopeLine2       { 0xffff6600 };
    juce::Colour scopeAxesText    { 0xff666666 };

    // ── Phase Correlation ────────────────────────────────────
    juce::Colour corrPositive     { 0xff00cc66 };
    juce::Colour corrNegative     { 0xffcc0000 };
    juce::Colour corrNeutral      { 0xff888888 };

    // ── LUFS Threshold LEDs ──────────────────────────────────
    juce::Colour lufsPass         { 0xff00cc66 };
    juce::Colour lufsWarn         { 0xffff6600 };
    juce::Colour lufsFail         { 0xffcc0000 };

    // ── Dropdowns / Combos ───────────────────────────────────
    juce::Colour comboBg          { 0xff2a2a2a };
    juce::Colour comboOutline     { 0xff555555 };
    juce::Colour comboText        { 0xffdddddd };
    juce::Colour comboArrow       { 0xffcc6600 };

    // ── Recording ────────────────────────────────────────────
    juce::Colour recordActive     { 0xffff0000 };
    juce::Colour recordInactive   { 0xff444444 };

    // ── Fonts ────────────────────────────────────────────────
    juce::String fontMain         { "Arial" };
    juce::String fontMono         { "Consolas" };
    float fontSize                { 13.0f };
    float fontSizeSmall           { 11.0f };
    float fontSizeTitle           { 15.0f };
    float fontSizeMono            { 11.0f };

    // ── Serum‑style metadata ─────────────────────────────────
    juce::File themeFolder;             // folder on disk (empty = built-in)
    juce::File previewImage;            // preview.png (may not exist)
    juce::Array<juce::File> fontFiles;   // .ttf files found in Fonts/

    // ── Helpers ──────────────────────────────────────────────
    juce::Colour meterColourForDB(float db) const;
};

// ============================================================
// UTTLookAndFeel — renders all controls with UTTTheme colours
// ============================================================
class UTTLookAndFeel : public juce::LookAndFeel_V4
{
public:
    UTTLookAndFeel();
    explicit UTTLookAndFeel(const UTTTheme& t);

    void setTheme(const UTTTheme& t);
    const UTTTheme& getTheme() const { return theme; }

    // ── Overrides ────────────────────────────────────────────
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
        float sliderPos, float startAngle, float endAngle,
        juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawAsHighlighted, bool shouldDrawAsDown) override;

    void drawComboBox(juce::Graphics& g, int w, int h, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& bg, bool isMouseOver, bool isDown) override;

    void drawGroupComponentOutline(juce::Graphics& g, int w, int h,
        const juce::String& text, const juce::Justification& position,
        juce::GroupComponent&) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void fillTextEditorBackground(juce::Graphics& g, int w, int h,
        juce::TextEditor&) override;

    void drawTextEditorOutline(juce::Graphics& g, int w, int h,
        juce::TextEditor&) override;

    // Font utilities
    juce::Font getMainFont(float size = -1.0f) const;
    juce::Font getMonoFont(float size = -1.0f) const;

protected:
    UTTTheme theme;
};


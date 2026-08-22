#include "UTTLookAndFeel.h"

// ================================================================
// meterColourForDB — returns colour for a dB level
// ================================================================
juce::Colour UTTTheme::meterColourForDB(float db) const
{
    if (db > -6.0f)  return meterHigh;           // red zone
    if (db > -12.0f) return meterMid;            // yellow zone
    if (db > -18.0f) return meterLow.interpolatedWith(meterMid, (db + 18.0f) / 6.0f);
    return meterLow;                              // green zone
}

// ================================================================
// UTTLookAndFeel constructors
// ================================================================
UTTLookAndFeel::UTTLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, theme.background);
    setDefaultSansSerifTypefaceName(theme.fontMain);
}

UTTLookAndFeel::UTTLookAndFeel(const UTTTheme& t) : theme(t)
{
    setColour(juce::ResizableWindow::backgroundColourId, theme.background);
    setDefaultSansSerifTypefaceName(theme.fontMain);
}

void UTTLookAndFeel::setTheme(const UTTTheme& t)
{
    // Theme colours are read directly from struct in draw methods — no setColour needed
    theme = t;
}

// ================================================================
// Font utilities
// ================================================================
juce::Font UTTLookAndFeel::getMainFont(float size) const
{
    float s = (size > 0.0f) ? size : theme.fontSize;
    return juce::Font(theme.fontMain, s, juce::Font::plain);
}

juce::Font UTTLookAndFeel::getMonoFont(float size) const
{
    float s = (size > 0.0f) ? size : theme.fontSizeMono;
    return juce::Font(theme.fontMono, s, juce::Font::plain);
}

// ================================================================
// Rotary Slider — hardware console knob style
// ================================================================
void UTTLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto rw = radius * 0.85f;
    auto angle = startAngle + sliderPos * (endAngle - startAngle);

    // Knob base (shadow ring)
    juce::Path basePath;
    basePath.addEllipse(cx - rw, cy - rw, rw * 2.0f, rw * 2.0f);
    g.setColour(theme.knobBase);
    g.fillPath(basePath);

    // Outer rim highlight
    g.setColour(theme.knobBase.brighter(0.15f));
    g.drawEllipse(cx - rw, cy - rw, rw * 2.0f, rw * 2.0f, 1.5f);

    // Background arc (the remaining range)
    float arcThickness = radius * 0.09f;
    juce::Path arcBg;
    arcBg.addCentredArc(cx, cy, rw - arcThickness, rw - arcThickness,
        0.0f, startAngle, endAngle, true);
    g.setColour(theme.knobArcBg);
    g.strokePath(arcBg, juce::PathStrokeType(arcThickness));

    // Active arc (the value portion)
    juce::Path arcVal;
    arcVal.addCentredArc(cx, cy, rw - arcThickness, rw - arcThickness,
        0.0f, startAngle, angle, true);
    g.setColour(theme.accent);
    g.strokePath(arcVal, juce::PathStrokeType(arcThickness));

    // Indicator line
    juce::Path indicator;
    float indLen = rw * 0.55f;
    indicator.addLineSegment(juce::Line<float>::fromStartAndAngle(juce::Point<float>(cx, cy), indLen, angle), 2.5f);
    g.setColour(theme.knobIndicator);
    g.strokePath(indicator, juce::PathStrokeType(2.5f));

    // Center dot
    g.setColour(theme.accent);
    g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
}

// ================================================================
// Toggle Button — LED indicator + text
// ================================================================
void UTTLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool, bool)
{
    auto area = button.getLocalBounds().reduced(2);
    auto isOn = button.getToggleState();

    float ledSize = 10.0f;
    float ledX = (float)area.getX() + 3.0f;
    float ledY = area.getCentreY() - ledSize * 0.5f;

    // Glow
    if (isOn)
    {
        g.setColour(theme.buttonGlow);
        g.fillEllipse(ledX - 2.0f, ledY - 2.0f, ledSize + 4.0f, ledSize + 4.0f);
    }

    // LED body
    g.setColour(isOn ? theme.buttonOn : theme.buttonOff);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    // Specular highlight
    if (isOn)
    {
        g.setColour(theme.buttonOn.brighter(0.4f).withAlpha(0.5f));
        g.fillEllipse(ledX + 2.0f, ledY + 1.5f, ledSize * 0.4f, ledSize * 0.35f);
    }

    // Text
    g.setColour(isOn ? theme.textPrimary : theme.textSecondary);
    g.setFont(getMainFont(theme.fontSizeSmall));
    g.drawText(button.getButtonText(),
               (int)(ledX + ledSize + 6), area.getY(),
               area.getWidth() - (int)(ledSize) - 10, area.getHeight(),
               juce::Justification::centredLeft);
}

// ================================================================
// ComboBox — custom dropdown
// ================================================================
void UTTLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool isDown,
    int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float>(0.0f, 0.0f, (float)w, (float)h).reduced(2.0f);

    g.setColour(isDown ? theme.comboBg.brighter(0.2f) : theme.comboBg);
    g.fillRoundedRectangle(area, 4.0f);

    g.setColour(isDown ? theme.accent : theme.comboOutline);
    g.drawRoundedRectangle(area, 4.0f, 1.5f);

    // Arrow
    float arrowX = area.getRight() - 16.0f;
    float arrowY = area.getCentreY();
    juce::Path arrow;
    arrow.addTriangle(arrowX - 4.0f, arrowY - 3.0f,
                      arrowX + 4.0f, arrowY - 3.0f,
                      arrowX,        arrowY + 3.0f);
    g.setColour(theme.comboArrow);
    g.fillPath(arrow);
}

void UTTLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(4, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont(getMainFont(theme.fontSizeSmall));
    label.setColour(juce::Label::textColourId, theme.comboText);
}

// ================================================================
// Labels
// ================================================================
void UTTLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), label.getLocalBounds(),
               label.getJustificationType(), false);
}

// ================================================================
// Button — accent coloured
// ================================================================
void UTTLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour&, bool isMouseOver, bool isDown)
{
    auto area = button.getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(isDown ? theme.accent.darker(0.3f)
               : isMouseOver ? theme.buttonHover
               : theme.accent);
    g.fillRoundedRectangle(area, 4.0f);

    g.setColour(theme.textPrimary);
    g.setFont(getMainFont(theme.fontSizeSmall));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred, false);
}

// ================================================================
// GroupComponent — section headers
// ================================================================
void UTTLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int w, int h,
    const juce::String& text, const juce::Justification&,
    juce::GroupComponent&)
{
    g.setColour(theme.border);
    g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1.0f, (float)h - 1.0f, 5.0f, 1.5f);

    // Text label at top
    if (text.isNotEmpty())
    {
        g.setColour(theme.textHighlight);
        g.setFont(getMainFont(theme.fontSize).boldened());
        g.drawText(text, 8, 3, w - 16, 20, juce::Justification::centredLeft);
    }
}

// ================================================================
// LinearSlider — horizontal sliders for phase correlation etc.
// ================================================================
void UTTLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float, float, const juce::Slider::SliderStyle,
    juce::Slider&)
{
    auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);

    // Track
    float trackH = area.getHeight() * 0.3f;
    float trackY = area.getCentreY() - trackH * 0.5f;
    auto track = area.withHeight(trackH).withY(trackY);

    g.setColour(theme.meterBg);
    g.fillRoundedRectangle(track, 2.0f);

    // Filled portion
    auto filled = track.withRight(track.getX() + sliderPos);
    g.setColour(theme.accent);
    g.fillRoundedRectangle(filled, 2.0f);

    // Thumb
    float thumbR = trackH * 1.2f;
    g.setColour(theme.accent.brighter(0.3f));
    g.fillEllipse(sliderPos - thumbR, area.getCentreY() - thumbR, thumbR * 2.0f, thumbR * 2.0f);
}

// ================================================================
// TextEditor — input fields
// ================================================================
void UTTLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int w, int h,
    juce::TextEditor&)
{
    g.setColour(theme.panelBg);
    g.fillRoundedRectangle(0.0f, 0.0f, (float)w, (float)h, 4.0f);
}

void UTTLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int w, int h,
    juce::TextEditor&)
{
    g.setColour(theme.border);
    g.drawRoundedRectangle(0.5f, 0.5f, (float)w - 1.0f, (float)h - 1.0f, 4.0f, 1.5f);
}



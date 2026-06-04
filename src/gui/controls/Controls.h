#pragma once
#include <JuceHeader.h>
#include <functional>
#include "../theme/UTTLookAndFeel.h"

// All controls are header-only to avoid cross-TU issues in VST3

// ============================================================
// LedButton — LED indicator + text label (theme‑aware)
// ============================================================
class LedButton : public juce::Component
{
public:
    juce::String text;
    bool isOn = false;
    std::function<void(bool)> onChange;
    const UTTTheme* theme = nullptr;

    LedButton() = default;
    explicit LedButton(const juce::String& t) : text(t) {}

    void setTheme(const UTTTheme* t) { theme = t; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds(); if (b.isEmpty()) return;
        auto bf = b.toFloat();
        float ls = 10.0f, lx = 4.0f;
        float ly = bf.getCentreY() - ls * 0.5f;

        juce::Colour ledOn  = theme ? theme->buttonOn  : juce::Colour(0xffff3300);
        juce::Colour ledOff = theme ? theme->buttonOff : juce::Colour(0xff2a1010);
        juce::Colour textOn = theme ? theme->textPrimary : juce::Colour(0xffe0e0e0);
        juce::Colour textOff = theme ? theme->textSecondary : juce::Colour(0xff888888);

        juce::Colour lc = isOn ? ledOn : ledOff;

        // Glow
        if (isOn) {
            auto glowCol = theme ? theme->buttonGlow : ledOn.withAlpha(0.25f);
            g.setColour(glowCol);
            g.fillEllipse(lx - 2, ly - 2, ls + 4, ls + 4);
        }

        // LED body
        juce::Path lp; lp.addEllipse(lx, ly, ls, ls);
        g.setColour(lc); g.fillPath(lp);

        // Specular highlight
        if (isOn) {
            g.setColour(lc.brighter(0.4f).withAlpha(0.4f));
            g.fillEllipse(lx + 1.5f, ly + 1.5f, ls * 0.35f, ls * 0.3f);
        }

        // Text
        g.setColour(isOn ? textOn : textOff);
        auto fontSize = theme ? theme->fontSizeSmall : 11.0f;
        g.setFont(fontSize);
        g.drawText(text, (int)(lx + ls + 6), 0,
                   b.getWidth() - (int)(ls) - 10, b.getHeight(),
                   juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        isOn = !isOn; repaint();
        if (onChange) onChange(isOn);
    }
    void setState(bool s) { isOn = s; repaint(); }
    bool getState() const { return isOn; }
};

// ============================================================
// VUKnob — Rotary knob with label and value display (theme‑aware)
// ============================================================
class VUKnob : public juce::Component
{
public:
    std::function<void(float)> onValueChanged;
    juce::Slider slider;
    juce::Label  nameLabel, valueLabel;
    juce::String suffix;
    const UTTTheme* theme = nullptr;

    VUKnob() = default;
    VUKnob(const juce::String& label, float minV, float maxV,
           const juce::String& sfx = "")
        : suffix(sfx)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange(minV, maxV);
        slider.setDoubleClickReturnValue(true,
            minV == -96.0f ? 0.0f : (minV + maxV) * 0.5f);
        addAndMakeVisible(slider);

        nameLabel.setText(label, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(11.0f);
        addAndMakeVisible(nameLabel);

        valueLabel.setText("0", juce::dontSendNotification);
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setFont(12.0f);
        valueLabel.setColour(juce::Label::textColourId,
                             juce::Colour(0xff00cc66));
        addAndMakeVisible(valueLabel);

        slider.onValueChange = [this]()
        {
            float v = (float)slider.getValue();
            if (suffix == "dB")
                valueLabel.setText(juce::String(v, 1) + " dB",
                                   juce::dontSendNotification);
            else if (suffix == "deg")
                valueLabel.setText(juce::String((int)v) + " deg",
                                   juce::dontSendNotification);
            else if (suffix == "%")
                valueLabel.setText(juce::String((int)v) + "%",
                                   juce::dontSendNotification);
            else
                valueLabel.setText(juce::String(v, 1),
                                   juce::dontSendNotification);
            if (onValueChanged) onValueChanged(v);
        };
    }

    void setTheme(const UTTTheme* t)
    {
        theme = t;
        if (t)
        {
            nameLabel.setColour(juce::Label::textColourId,
                                t->textSecondary);
            valueLabel.setColour(juce::Label::textColourId,
                                 t->knobValueText);
            nameLabel.setFont(t->fontSizeSmall);
            valueLabel.setFont(t->fontSizeSmall + 1.0f);
        }
        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(16));
        slider.setBounds(b.getX(), b.getY(), b.getWidth(),
                         b.getHeight() - 20);
        valueLabel.setBounds(b.removeFromBottom(18));
    }

    float getValue() const { return (float)slider.getValue(); }
    void setValue(float v) { slider.setValue(v); }
};

// ============================================================
// VUMeter — Dual‑channel VU meter bar (theme‑aware)
// ============================================================
class VUMeterBar : public juce::Component
{
public:
    bool isLeft = true;
    const UTTTheme* theme = nullptr;
    float peakLevel = -96.0f, rmsLevel = -96.0f, peakHold = -96.0f;

    VUMeterBar() = default;

    void setLevel(float pDB, float rDB)
    {
        peakLevel  = juce::jlimit(-96.0f, 6.0f, pDB);
        rmsLevel   = juce::jlimit(-96.0f, 6.0f, rDB);
        if (peakLevel > peakHold) peakHold = peakLevel;
        repaint();
    }

    void setTheme(const UTTTheme* t) { theme = t; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds(); if (b.isEmpty()) return;
        auto bf = b.toFloat();
        float mw = b.getWidth() * 0.7f;
        float mx = (b.getWidth() - mw) * 0.5f;

        auto ma = bf.withX(mx).withWidth(mw);

        auto bg     = theme ? theme->meterBg   : juce::Colour(0xff0a0a0a);
        auto tick   = theme ? theme->meterTick : juce::Colour(0xff666666);
        auto hold   = theme ? theme->meterPeakHold : juce::Colours::white;
        auto low    = theme ? theme->meterLow  : juce::Colour(0xff00cc00);
        auto mid    = theme ? theme->meterMid  : juce::Colour(0xffcccc00);
        auto high   = theme ? theme->meterHigh : juce::Colour(0xffcc0000);

        // Background
        g.setColour(bg);
        g.fillRect(ma);

        // Tick marks
        g.setFont(9.0f);
        for (int db = 0; db >= -96; db -= 6)
        {
            float y = juce::jmap((float)db, -96.0f, 6.0f,
                                 bf.getHeight() - 4.0f, 4.0f);
            g.setColour(tick);
            g.drawLine(ma.getX() + mw * 0.15f, y,
                       ma.getRight(), y, 0.5f);
            if (db % 12 == 0)
            {
                g.setColour(tick.brighter(0.2f));
                g.drawText(juce::String(db),
                           (int)(ma.getRight() + 2),
                           (int)(y - 6),
                           (int)(bf.getWidth() - ma.getRight() - 2),
                           12, juce::Justification::left);
                g.drawLine(ma.getX(), y, ma.getRight(), y, 0.5f);
            }
        }

        // RMS level bar
        float rmsY = juce::jmap(rmsLevel, -96.0f, 6.0f,
                                bf.getHeight() - 4.0f, 4.0f);
        auto rmsBar = ma.withTop(rmsY);
        if (rmsBar.getHeight() > 0)
        {
            juce::ColourGradient grad(low, rmsBar.getX(),
                                      rmsBar.getBottom(),
                                      high, rmsBar.getX(),
                                      rmsBar.getY(), false);
            grad.addColour(0.4f, mid);
            g.setGradientFill(grad);
            g.fillRect(rmsBar.reduced(1.0f, 0));
        }

        // Peak hold line
        float phY = juce::jmap(peakHold, -96.0f, 6.0f,
                               bf.getHeight() - 4.0f, 4.0f);
        g.setColour(hold);
        g.drawLine(ma.getX(), phY, ma.getRight(), phY, 1.5f);

        // Channel label
        g.setColour(tick);
        g.setFont(8.0f);
        g.drawText(isLeft ? "L" : "R", 0, (int)(bf.getHeight() - 12),
                   (int)ma.getX(), 12, juce::Justification::centred);
    }
};

// ============================================================
// Goniometer / Oscilloscope (theme‑aware)
// ============================================================
class UTTScope : public juce::Component
{
public:
    enum Mode { Goniometer, Oscilloscope };
    Mode mode = Goniometer;
    const UTTTheme* theme = nullptr;

    // Oscilloscope data: array of {L, R} sample pairs
    std::vector<juce::Point<float>> points;

    void setPoints(const std::vector<juce::Point<float>>& pts)
    {
        points = pts; repaint();
    }

    void setTheme(const UTTTheme* t) { theme = t; repaint(); }
    void setMode(Mode m) { mode = m; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds(); if (b.isEmpty()) return;
        auto bf = b.toFloat();

        auto bg      = theme ? theme->scopeBg      : juce::Colour(0xff0d0d0d);
        auto grid    = theme ? theme->scopeGrid    : juce::Colour(0xff1a1a1a);
        auto line1   = theme ? theme->scopeLine    : juce::Colour(0xff00ff66);
        auto line2   = theme ? theme->scopeLine2   : juce::Colour(0xffff6600);
        auto axes    = theme ? theme->scopeAxesText : juce::Colour(0xff666666);

        g.setColour(bg);
        g.fillRect(bf);

        if (mode == Goniometer)
            paintGoniometer(g, bf, grid, line1, axes);
        else
            paintOscilloscope(g, bf, grid, line1, line2, axes);
    }

private:
    void paintGoniometer(juce::Graphics& g, juce::Rectangle<float> bf,
                         juce::Colour grid, juce::Colour line,
                         juce::Colour axes)
    {
        float w = bf.getWidth(), h = bf.getHeight();
        float cx = w * 0.5f, cy = h * 0.5f;
        float scale = juce::jmin(w, h) * 0.45f;

        // Grid: circle + cross + diagonals
        g.setColour(grid);
        g.drawEllipse(cx - scale, cy - scale, scale * 2, scale * 2, 1.0f);
        g.drawLine(cx,       cy - scale, cx,       cy + scale, 0.5f);
        g.drawLine(cx - scale, cy,       cx + scale, cy,       0.5f);
        // 45° diagonals (L/R axes)
        float d45 = scale * 0.707f;
        g.drawLine(cx - d45, cy + d45, cx + d45, cy - d45, 0.5f);
        g.drawLine(cx - d45, cy - d45, cx + d45, cy + d45, 0.5f);

        // Axis labels
        g.setColour(axes);
        g.setFont(9.0f);
        g.drawText("L", cx - scale - 14, cy - 6, 14, 12,
                   juce::Justification::centred);
        g.drawText("R", cx + scale + 2,  cy - 6, 14, 12,
                   juce::Justification::centred);
        g.drawText("M", cx - 6, 2, 12, 12,
                   juce::Justification::centred);
        g.drawText("S", w - 18, cy - 6, 14, 12,
                   juce::Justification::centred);

        // Mode label
        g.setColour(axes);
        g.setFont(9.0f);
        g.drawText("GONIOMETER", 4, 2, 80, 12,
                   juce::Justification::centredLeft);

        if (points.empty()) return;

        // Draw trace with CRT fade effect
        // We accumulate paths and draw with decreasing alpha for phosphor trail
        juce::Path path;
        bool first = true;
        for (size_t i = 0; i < points.size(); i += 2)
        {
            float L = points[i].x;
            float R = points[i].y;
            float x = (L - R) * scale * 0.7f;
            float y = -(L + R) * scale * 0.7f;
            float dx = juce::jlimit(2.0f, w - 2.0f, cx + x);
            float dy = juce::jlimit(2.0f, h - 2.0f, cy + y);
            if (first) { path.startNewSubPath(dx, dy); first = false; }
            else path.lineTo(dx, dy);
        }
        g.setColour(line.withAlpha(0.8f));
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }

    void paintOscilloscope(juce::Graphics& g, juce::Rectangle<float> bf,
                           juce::Colour grid, juce::Colour line1,
                           juce::Colour line2, juce::Colour axes)
    {
        float w = bf.getWidth(), h = bf.getHeight();

        // Grid
        g.setColour(grid);
        for (int i = 1; i < 8; ++i)
        {
            float gy = h * i / 8.0f;
            g.drawLine(0, gy, w, gy, 0.5f);
        }
        for (int i = 1; i < 10; ++i)
        {
            float gx = w * i / 10.0f;
            g.drawLine(gx, 0, gx, h, 0.5f);
        }

        // Center line
        g.setColour(grid.brighter(0.2f));
        g.drawLine(0, h * 0.5f, w, h * 0.5f, 1.0f);

        // Mode label
        g.setColour(axes);
        g.setFont(9.0f);
        g.drawText("OSCILLOSCOPE", 4, 2, 80, 12,
                   juce::Justification::centredLeft);

        if (points.empty()) return;

        float midY = h * 0.5f;
        int step = juce::jmax(1, (int)points.size() / (int)w);

        // L (line1 color)
        juce::Path pathL; bool firstL = true;
        for (size_t i = 0; i < points.size(); i += step)
        {
            float x = juce::jmap((float)i / points.size(), 0.0f, w);
            float y = juce::jlimit(2.0f, h - 2.0f,
                                   midY - points[i].x * midY * 0.8f);
            if (firstL) { pathL.startNewSubPath(x, y); firstL = false; }
            else pathL.lineTo(x, y);
        }
        g.setColour(line1.withAlpha(0.8f));
        g.strokePath(pathL, juce::PathStrokeType(1.5f));

        // R (line2 color)
        juce::Path pathR; bool firstR = true;
        for (size_t i = 0; i < points.size(); i += step)
        {
            float x = juce::jmap((float)i / points.size(), 0.0f, w);
            float y = juce::jlimit(2.0f, h - 2.0f,
                                   midY - points[i].y * midY * 0.8f);
            if (firstR) { pathR.startNewSubPath(x, y); firstR = false; }
            else pathR.lineTo(x, y);
        }
        g.setColour(line2.withAlpha(0.8f));
        g.strokePath(pathR, juce::PathStrokeType(1.5f));

        // Legend
        g.setColour(line1);
        g.setFont(8.0f);
        g.drawText("L", (int)(w - 24), 12, 10, 10,
                   juce::Justification::centred);
        g.setColour(line2);
        g.drawText("R", (int)(w - 24), 22, 10, 10,
                   juce::Justification::centred);
    }
};

// ============================================================
// SegmentDisplay — LED‑style numeric display
// ============================================================
class SegmentDisplay : public juce::Component
{
public:
    const UTTTheme* theme = nullptr;

    SegmentDisplay() = default;
    explicit SegmentDisplay(int nd) : numDigits(nd)
        { setSize(nd * 22 + 10, 30); }

    void setValue(float v) { curVal = v; repaint(); }
    void setTheme(const UTTTheme* t) { theme = t; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds(); if (b.isEmpty()) return;
        auto bf = b.toFloat().reduced(2.0f);

        auto bg  = theme ? theme->meterBg : juce::Colour(0xff111111);
        auto brd = theme ? theme->border  : juce::Colour(0xff444444);
        auto txt = theme ? theme->knobValueText : juce::Colour(0xffffcc00);

        g.setColour(bg);
        g.fillRoundedRectangle(bf, 3.0f);
        g.setColour(brd);
        g.drawRoundedRectangle(bf, 3.0f, 1.0f);

        auto fontName = theme ? theme->fontMono : "Consolas";
        g.setColour(txt);
        g.setFont(juce::Font(fontName, 18.0f, juce::Font::bold));
        g.drawText(juce::String(curVal, 1), bf,
                   juce::Justification::centred);
    }

private:
    int numDigits = 3;
    float curVal = 0.0f;
};

// ============================================================
// PhaseCorrelationBar — horizontal bar showing L/R correlation
// ============================================================
class PhaseCorrelationBar : public juce::Component
{
public:
    const UTTTheme* theme = nullptr;
    float correlation = 1.0f;

    void setCorrelation(float c) { correlation = c; repaint(); }
    void setTheme(const UTTTheme* t) { theme = t; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds(); if (b.isEmpty()) return;
        auto bf = b.toFloat();
        float barH = bf.getHeight() * 0.35f;
        float barY = bf.getCentreY() - barH * 0.5f;
        auto bar = bf.withHeight(barH).withY(barY);

        auto pos   = theme ? theme->corrPositive : juce::Colour(0xff00cc66);
        auto neg   = theme ? theme->corrNegative : juce::Colour(0xffcc0000);
        auto neut  = theme ? theme->corrNeutral  : juce::Colour(0xff888888);
        auto bg    = theme ? theme->meterBg      : juce::Colour(0xff111111);
        auto text  = theme ? theme->textSecondary: juce::Colour(0xffaaaaaa);

        // Background track
        g.setColour(bg);
        g.fillRoundedRectangle(bar, 2.0f);

        // Fill from center to current
        float centerX = bar.getCentreX();
        float dotX = juce::jmap(correlation, -1.0f, 1.0f,
                                bar.getX(), bar.getRight());
        juce::Colour fillCol = (correlation >= 0.0f) ? pos : neg;
        auto filled = juce::Rectangle<float>(
            juce::jmin(centerX, dotX), bar.getY(),
            std::abs(dotX - centerX), barH);
        if (filled.getWidth() > 0)
        {
            g.setColour(fillCol);
            g.fillRoundedRectangle(filled, 2.0f);
        }

        // Center line
        g.setColour(neut);
        g.drawLine(centerX, bar.getY(), centerX, bar.getBottom(), 1.5f);

        // Indicator dot
        float dotR = barH * 0.6f;
        g.setColour(fillCol.brighter(0.3f));
        g.fillEllipse(dotX - dotR, bf.getCentreY() - dotR,
                      dotR * 2.0f, dotR * 2.0f);

        // Labels
        g.setColour(text);
        g.setFont(theme ? theme->fontSizeSmall : 10.0f);
        g.drawText("-1 (Out of Phase)", (int)bar.getX(), 0,
                   (int)(centerX - bar.getX()), (int)bf.getY(),
                   juce::Justification::topLeft);
        g.drawText("+1 (In Phase)", (int)centerX, 0,
                   (int)(bar.getRight() - centerX), (int)bf.getY(),
                   juce::Justification::topRight);

        // Value
        g.setColour(text);
        g.setFont(juce::Font(theme ? theme->fontMono : "Consolas",
                             theme ? theme->fontSizeMono : 11.0f,
                             juce::Font::bold));
        g.drawText("Phase: " + juce::String(correlation, 3),
                   bar.withY(bar.getBottom() + 2),
                   juce::Justification::centredTop);
    }
};

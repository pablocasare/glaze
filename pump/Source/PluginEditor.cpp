#include "PluginEditor.h"
#include "BinaryData.h"

using namespace pumpui;
using juce::Colour;
using juce::Rectangle;

namespace c
{
    const Colour bg     { 0xff0d0c12 };
    const Colour shell  { 0xff19171f };
    const Colour shell2 { 0xff24212d };
    const Colour edge   { 0xff36313f };
    const Colour lime   { 0xffc6ff3d };
    const Colour violet { 0xff9b6bff };
    const Colour text   { 0xffeeeef2 };
    const Colour dim    { 0xff8a8597 };
    const Colour screen { 0xff111016 };
}

static LnF* gLnf = nullptr;
static juce::Font monoF (float h) { return juce::Font (juce::FontOptions().withTypeface (gLnf->mono).withHeight (h)); }
static juce::Font logoF (float h) { return juce::Font (juce::FontOptions().withTypeface (gLnf->logo).withHeight (h)); }

LnF::LnF()
{
    logo = juce::Typeface::createSystemTypefaceFor (BinaryData::RighteousRegular_ttf, BinaryData::RighteousRegular_ttfSize);
    mono = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    setColour (juce::PopupMenu::backgroundColourId, c::shell2);
    setColour (juce::PopupMenu::textColourId, c::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, c::lime.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, c::lime);
    setColour (juce::ComboBox::textColourId, c::text);
    setColour (juce::Slider::textBoxTextColourId, c::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, c::bg);
    setColour (juce::TextEditor::textColourId, c::text);
    setColour (juce::CaretComponent::caretColourId, c::lime);
}

void LnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    const float rad = std::min (r.getWidth(), r.getHeight()) * 0.5f;
    const auto ctr = r.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const bool active = s.isEnabled();
    const Colour acc = active ? c::lime : c::dim.withAlpha (0.5f);

    juce::Path track; track.addCentredArc (ctr.x, ctr.y, rad * 0.9f, rad * 0.9f, 0, a0, a1, true);
    g.setColour (c::edge); g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc (ctr.x, ctr.y, rad * 0.9f, rad * 0.9f, 0, a0, ang, true);
    if (active) { g.setColour (acc.withAlpha (0.25f)); g.strokePath (val, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (acc); g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = rad * 0.66f;
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (ctr.x - br, ctr.y - br + 3, br * 2, br * 2);
    g.setGradientFill (juce::ColourGradient (Colour (0xff3a3645), ctr.x, ctr.y - br, Colour (0xff1a1820), ctr.x, ctr.y + br, false));
    g.fillEllipse (ctr.x - br, ctr.y - br, br * 2, br * 2);
    g.setColour (s.isMouseOverOrDragging() ? acc.withAlpha (0.7f) : c::edge); g.drawEllipse (ctr.x - br, ctr.y - br, br * 2, br * 2, 1.2f);
    const auto p1 = ctr.getPointOnCircumference (br * 0.3f, ang), p2 = ctr.getPointOnCircumference (br * 0.85f, ang);
    g.setColour (active ? c::text : c::dim); g.drawLine ({ p1, p2 }, 2.4f);
}

void LnF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& b)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (c::screen); g.fillRoundedRectangle (r, 6);
    g.setColour (b.isMouseOver() ? c::lime : c::edge); g.drawRoundedRectangle (r, 6, 1);
    juce::Path a; const float ax = (float) w - 14, ay = (float) h * 0.5f;
    a.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (c::lime); g.fillPath (a);
}
juce::Font LnF::getComboBoxFont (juce::ComboBox&) { return monoF (12.5f); }
juce::Font LnF::getPopupMenuFont() { return monoF (13.0f); }
void LnF::positionComboBoxText (juce::ComboBox& b, juce::Label& l)
{
    l.setBounds (8, 0, b.getWidth() - 26, b.getHeight()); l.setFont (getComboBoxFont (b));
}
juce::Label* LnF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (monoF (12.0f)); l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

// ---------------------------------------------------------------------------
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& lbl) : label (lbl)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    slider.setMouseDragSensitivity (200);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    auto* p = s.getParameter (id);
    slider.textFromValueFunction = [p] (double v)
    {
        const auto u = p->getLabel();
        if (u == "%")  return juce::String (juce::roundToInt (v)) + " %";
        if (u == "dB") return juce::String (v, 1) + " dB";
        return (v > 0.5 ? "+" : "") + juce::String (juce::roundToInt (v));
    };
    slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.updateText();
}
void Knob::paint (juce::Graphics& g)
{
    g.setFont (monoF (10.5f));
    g.setColour (isEnabled() ? c::dim : c::dim.withAlpha (0.4f));
    g.drawText (label, getLocalBounds().removeFromTop (16), juce::Justification::centred);
}

// ---------------------------------------------------------------------------
ModeSwitch::ModeSwitch (juce::AudioProcessorValueTreeState& st) : s (st) { s.addParameterListener ("mode", this); }
ModeSwitch::~ModeSwitch() { s.removeParameterListener ("mode", this); }
void ModeSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (c::screen); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (c::edge); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1);
    const int sel = s.getRawParameterValue ("mode")->load() > 0.5f ? 1 : 0;
    auto half = r.reduced (3).withWidth ((r.getWidth() - 6) * 0.5f).translated (sel * (r.getWidth() - 6) * 0.5f, 0);
    g.setColour (c::lime); g.fillRoundedRectangle (half, half.getHeight() * 0.5f);
    g.setFont (monoF (11.5f));
    const char* names[] { "TEMPO", "SIDECHAIN" };
    for (int i = 0; i < 2; ++i)
    {
        g.setColour (i == sel ? c::bg : c::dim);
        g.drawText (names[i], r.withWidth (r.getWidth() * 0.5f).translated (i * r.getWidth() * 0.5f, 0), juce::Justification::centred);
    }
}
void ModeSwitch::mouseDown (const juce::MouseEvent& e)
{
    auto* p = s.getParameter ("mode");
    p->beginChangeGesture();
    p->setValueNotifyingHost (e.x > getWidth() / 2 ? 1.0f : 0.0f);
    p->endChangeGesture();
    repaint();
}

// ---------------------------------------------------------------------------
Rectangle<float> Display::plot() const { return getLocalBounds().toFloat().reduced (16, 14).withTrimmedTop (22).withTrimmedBottom (4); }

void Display::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (c::screen); g.fillRoundedRectangle (r, 14);
    g.setColour (c::edge); g.drawRoundedRectangle (r.reduced (0.5f), 14, 1);

    auto& ap = proc.apvts;
    auto v = [&] (const char* id) { return ap.getRawParameterValue (id)->load(); };
    const float depth = v ("depth") * 0.01f, length = v ("length") * 0.01f, curve = v ("curve") * 0.01f;
    const bool scMode = v ("mode") > 0.5f;
    const int rateIdx = (int) v ("rate");

    auto pl = plot();
    // grid: 4 sub-divisions + dB lines
    for (int i = 0; i <= 4; ++i)
    {
        const float x = pl.getX() + pl.getWidth() * i / 4.0f;
        g.setColour (juce::Colours::white.withAlpha (i == 0 || i == 4 ? 0.10f : 0.04f));
        g.drawVerticalLine ((int) x, pl.getY(), pl.getBottom());
    }
    for (float frac : { 0.25f, 0.5f, 0.75f })
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.drawHorizontalLine ((int) (pl.getBottom() - pl.getHeight() * frac), pl.getX(), pl.getRight());
    }

    // curve
    juce::Path line, fill;
    for (int i = 0; i <= 400; ++i)
    {
        const float ph = i / 400.0f;
        const float gn = pump::shapeGain (ph, depth, length, curve);
        const float x = pl.getX() + ph * pl.getWidth(), y = pl.getBottom() - gn * pl.getHeight();
        if (i == 0) { line.startNewSubPath (x, y); fill.startNewSubPath (x, pl.getBottom()); }
        else line.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (pl.getRight(), pl.getBottom()); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c::lime.withAlpha (0.35f), 0, pl.getY(), c::violet.withAlpha (0.08f), 0, pl.getBottom(), false));
    g.fillPath (fill);
    g.setColour (c::lime.withAlpha (0.25f)); g.strokePath (line, juce::PathStrokeType (8.0f));
    g.setColour (c::lime); g.strokePath (line, juce::PathStrokeType (2.5f));

    // handles: depth (bottom of the dip) and length (where it's fully back)
    for (int h = 0; h < 2; ++h)
    {
        const auto pt = handlePos (h);
        const bool hot = h == active;
        g.setColour (c::violet.withAlpha (hot ? 0.5f : 0.3f)); g.fillEllipse (pt.x - 10, pt.y - 10, 20, 20);
        g.setColour (c::violet); g.fillEllipse (pt.x - 6, pt.y - 6, 12, 12);
        g.setColour (c::text); g.drawEllipse (pt.x - 6, pt.y - 6, 12, 12, 1.5f);
    }

    // playhead
    const float ph = proc.uiPhase.load(), gn = proc.uiGain.load();
    if (ph < 1.0f)
    {
        const float x = pl.getX() + ph * pl.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawVerticalLine ((int) x, pl.getY(), pl.getBottom());
        const float y = pl.getBottom() - gn * pl.getHeight();
        g.setColour (juce::Colours::white); g.fillEllipse (x - 5, y - 5, 10, 10);
    }

    // captions
    auto top = getLocalBounds().toFloat().reduced (16, 10).removeFromTop (18);
    g.setFont (monoF (11.5f));
    g.setColour (c::text);
    const float duckDb = juce::Decibels::gainToDecibels (gn, -60.0f);
    g.drawText ("DUCK " + juce::String (duckDb, 1) + " dB", top, juce::Justification::centredLeft);
    g.setColour (c::dim);
    g.drawText (pump::rateNames[rateIdx] + "  |  " + juce::String (juce::roundToInt (proc.bpm.load())) + " BPM", top, juce::Justification::centredRight);

    if (scMode)
    {
        // sidechain level vs threshold strip on the right edge
        auto strip = Rectangle<float> (pl.getRight() - 8, pl.getY(), 6, pl.getHeight());
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (strip, 3);
        auto yOf = [&] (float db) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, strip.getBottom(), strip.getY()); };
        g.setColour (c::violet); g.fillRect (strip.withTop (yOf (scLevel)));
        g.setColour (c::lime); g.fillRect (strip.getX() - 4, yOf (v ("thresh")) - 1, 14.0f, 2.0f);
        if (! proc.uiScConnected.load())
        {
            g.setColour (c::bg.withAlpha (0.75f)); g.fillRoundedRectangle (pl.withSizeKeepingCentre (380, 46), 8);
            g.setColour (c::lime); g.setFont (monoF (12.0f));
            g.drawText ("NO SIDECHAIN INPUT", pl.withSizeKeepingCentre (380, 46).removeFromTop (24), juce::Justification::centredBottom);
            g.setColour (c::dim); g.setFont (monoF (10.5f));
            g.drawText ("route your kick to Pump's sidechain input", pl.withSizeKeepingCentre (380, 46).removeFromBottom (22), juce::Justification::centredTop);
        }
    }
    else
    {
        g.setFont (monoF (9.5f)); g.setColour (c::dim.withAlpha (0.7f));
        g.drawText ("DRAG THE DOTS  |  DOUBLE-CLICK = RESET", pl.removeFromBottom (14), juce::Justification::centredRight);
    }
}

juce::Point<float> Display::handlePos (int h) const
{
    auto pl = plot();
    auto& ap = proc.apvts;
    const float depth = ap.getRawParameterValue ("depth")->load() * 0.01f, length = ap.getRawParameterValue ("length")->load() * 0.01f;
    if (h == 0) return { pl.getX() + pl.getWidth() * 0.03f, pl.getBottom() - (1.0f - depth) * pl.getHeight() };
    return { pl.getX() + length * pl.getWidth(), pl.getY() };
}
void Display::mouseDown (const juce::MouseEvent& e)
{
    active = -1; float best = 22.0f;
    for (int h = 0; h < 2; ++h) { const float d = handlePos (h).getDistanceFrom (e.position); if (d < best) { best = d; active = h; } }
    if (active < 0) // clicked elsewhere: horizontal-ish clicks move length, others depth
        active = std::abs (e.position.y - plot().getY()) < 30.0f ? 1 : 0;
    proc.apvts.getParameter (active == 0 ? "depth" : "length")->beginChangeGesture();
    mouseDrag (e);
}
void Display::mouseDrag (const juce::MouseEvent& e)
{
    if (active < 0) return;
    auto pl = plot();
    auto set = [&] (const char* id, float val) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (val)); };
    if (active == 0) set ("depth", 100.0f * juce::jlimit (0.0f, 1.0f, 1.0f - (pl.getBottom() - e.position.y) / pl.getHeight()));
    else             set ("length", 100.0f * juce::jlimit (0.05f, 1.0f, (e.position.x - pl.getX()) / pl.getWidth()));
    repaint();
}
void Display::mouseUp (const juce::MouseEvent&)
{
    if (active >= 0) proc.apvts.getParameter (active == 0 ? "depth" : "length")->endChangeGesture();
    active = -1;
}
void Display::mouseDoubleClick (const juce::MouseEvent&)
{
    for (auto id : { "depth", "length", "curve" }) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->getDefaultValue()); }
}

// ---------------------------------------------------------------------------
PumpEditor::PumpEditor (PumpProcessor& p) : AudioProcessorEditor (p), proc (p), display (p), mode (p.apvts)
{
    gLnf = &lnf;
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    setLookAndFeel (&lnf);
    addAndMakeVisible (display);
    addAndMakeVisible (mode);
    rate.addItemList (pump::rateNames, 1);
    rateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "rate", rate);
    addAndMakeVisible (rate);
    for (auto [id, name] : { std::pair { "depth", "DEPTH" }, { "length", "LENGTH" }, { "curve", "CURVE" }, { "thresh", "THRESHOLD" }, { "mix", "MIX" } })
    {
        knobs.push_back (std::make_unique<pumpui::Knob> (p.apvts, id, name));
        addAndMakeVisible (*knobs.back());
    }
    setSize (640, 430);
    startTimerHz (60);
}

PumpEditor::~PumpEditor() { stopTimer(); juce::LookAndFeel::setDefaultLookAndFeel (nullptr); setLookAndFeel (nullptr); }

void PumpEditor::timerCallback()
{
    display.scLevel = proc.uiScLevelDb.load() > display.scLevel ? proc.uiScLevelDb.load() : display.scLevel - 1.5f;
    display.repaint();
    const bool sc = proc.apvts.getRawParameterValue ("mode")->load() > 0.5f;
    if (knobs[3]->isEnabled() != sc) { knobs[3]->setEnabled (sc); knobs[3]->repaint(); }
}

void PumpEditor::paint (juce::Graphics& g)
{
    g.fillAll (c::bg);
    auto r = getLocalBounds().toFloat().reduced (5);
    g.setGradientFill (juce::ColourGradient (c::shell2, 0, r.getY(), c::shell, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 18);
    g.setColour (c::edge); g.drawRoundedRectangle (r, 18, 1.2f);

    juce::GlyphArrangement ga; ga.addLineOfText (logoF (38.0f), "pump", 0, 0);
    juce::Path lp; ga.createPath (lp);
    lp.applyTransform (juce::AffineTransform::shear (-0.15f, 0));
    auto b = lp.getBounds();
    lp.applyTransform (juce::AffineTransform::translation (26 - b.getX(), 36 - b.getCentreY()));
    juce::DropShadow (c::lime.withAlpha (0.5f), 14, {}).drawForPath (g, lp);
    g.setColour (c::lime); g.fillPath (lp);
}

void PumpEditor::resized()
{
    mode.setBounds (getWidth() - 400, 20, 220, 32);
    rate.setBounds (getWidth() - 166, 20, 140, 32);
    display.setBounds (20, 66, getWidth() - 40, 210);
    const int kw = (getWidth() - 40) / (int) knobs.size();
    for (size_t i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (20 + (int) i * kw, 292, kw, 118);
}

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
// Stage: the pump + inflatable duck cartoon
namespace c2
{
    const Colour duck     { 0xffffd93d };
    const Colour duckDark { 0xffe0a800 };
    const Colour beak     { 0xffff8a1f };
    const Colour pumpRed  { 0xffff4d5e };
    const Colour metal    { 0xffb9bcc8 };
}

void Stage::stroke()
{
    if (isPopping()) return;
    ++strokes;
    inflateTarget = juce::jmin (1.0f, (float) strokes / strokesToPop);
    squashVel += 2.2f;
    for (int i = 0; i < 3; ++i) puffs.push_back ({ -0.12f * (float) i, 0.6f + 0.4f * (float) i / 3.0f });
    if (strokes >= strokesToPop) pop();
}

void Stage::pop()
{
    if (isPopping()) return;
    popAge = 0;
    auto v = valvePos();
    const auto ctr = juce::Point<float> (duckX + 20.0f, v.y - 25.0f);
    juce::Random r;
    for (int i = 0; i < 26; ++i)
    {
        const float a = r.nextFloat() * juce::MathConstants<float>::twoPi;
        const float sp = 140.0f + r.nextFloat() * 260.0f;
        const juce::Colour cols[] { c2::duck, c2::duckDark, c2::beak, c::lime, c::violet };
        bits.push_back ({ ctr, { std::cos (a) * sp, std::sin (a) * sp - 120.0f }, r.nextFloat() * 6.0f, (r.nextFloat() - 0.5f) * 20.0f, 1.0f, cols[i % 5] });
    }
}

void Stage::tick (float dt)
{
    time += dt;
    const float phase = proc.uiPhase.load(), gain = proc.uiGain.load();
    const float depth = juce::jmax (0.05f, proc.apvts.getRawParameterValue ("depth")->load() * 0.01f);
    // new cycle (tempo wrap or sidechain retrigger) = one pump stroke
    if (phase < lastPhase - 0.3f || (lastPhase >= 1.0f && phase < 0.5f)) stroke();
    lastPhase = phase;

    const float down = juce::jlimit (0.0f, 1.0f, (1.0f - gain) / depth);
    handle += (down - handle) * juce::jmin (1.0f, dt * 30.0f);
    gauge  += (juce::jlimit (0.0f, 1.0f, (1.0f - gain)) - gauge) * juce::jmin (1.0f, dt * 12.0f);

    // squash spring
    squashVel += (-60.0f * squash - 7.0f * squashVel) * dt;
    squash += squashVel * dt;

    if (isPopping())
    {
        popAge += dt;
        if (popAge > 1.1f) { popAge = -1; strokes = 0; inflate = 0; inflateTarget = 0; regrow = 0; }
    }
    regrow = juce::jmin (1.0f, regrow + dt * 3.0f);
    inflate += (inflateTarget - inflate) * juce::jmin (1.0f, dt * 8.0f);

    for (auto& pf : puffs) pf.t += dt * 2.6f;
    puffs.erase (std::remove_if (puffs.begin(), puffs.end(), [] (const Puff& pf) { return pf.t > 1.0f; }), puffs.end());
    for (auto& b : bits) { b.vel.y += 600.0f * dt; b.pos += b.vel * dt; b.rot += b.spin * dt; b.life -= dt * 1.1f; }
    bits.erase (std::remove_if (bits.begin(), bits.end(), [] (const Bit& b) { return b.life <= 0; }), bits.end());
}

juce::Point<float> Stage::valvePos() const
{
    const float ground = (float) getHeight() - 22.0f;
    const float sc = (0.45f + 0.55f * inflate) * regrow;
    return { duckX - 72.0f * 0.87f * sc, ground - 72.0f * 0.5f * sc };
}

void Stage::drawPump (juce::Graphics& g, Rectangle<float> area)
{
    const float ground = area.getBottom();
    const float bx = area.getX() + 70.0f;          // barrel centre x
    const float barrelTop = ground - 104.0f;
    const float travel = 34.0f;
    const float handleY = barrelTop - 44.0f + handle * travel;

    // shadow + base plate
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (bx - 62, ground - 5, 124, 12);
    g.setColour (Colour (0xff2c2a33));
    g.fillRoundedRectangle (bx - 52, ground - 12, 104, 12, 5);
    g.setColour (Colour (0xff45424f));
    g.fillRoundedRectangle (bx - 52, ground - 12, 104, 4, 2);

    // shaft
    g.setColour (c2::metal);
    g.fillRect (bx - 3.0f, handleY, 6.0f, barrelTop - handleY + 4);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.fillRect (bx - 2.0f, handleY, 1.5f, barrelTop - handleY + 4);

    // barrel
    auto barrel = Rectangle<float> (bx - 17, barrelTop, 34, ground - 12 - barrelTop);
    g.setGradientFill (juce::ColourGradient (c2::pumpRed.brighter (0.25f), barrel.getX(), 0, c2::pumpRed.darker (0.5f), barrel.getRight(), 0, false));
    g.fillRoundedRectangle (barrel, 8);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillRoundedRectangle (barrel.getX() + 5, barrel.getY() + 8, 4, barrel.getHeight() - 16, 2);
    g.setColour (Colour (0xff2c2a33));
    g.fillRoundedRectangle (bx - 20, barrelTop - 4, 40, 8, 3);

    // gauge (needle = how hard it's ducking)
    const auto gc = juce::Point<float> (bx, barrelTop + 34);
    g.setColour (juce::Colours::white); g.fillEllipse (gc.x - 13, gc.y - 13, 26, 26);
    g.setColour (Colour (0xff2c2a33)); g.drawEllipse (gc.x - 13, gc.y - 13, 26, 26, 2.5f);
    for (int i = 0; i <= 6; ++i)
    {
        const float a = juce::jmap ((float) i / 6.0f, -2.2f, 2.2f);
        g.setColour (i >= 5 ? c2::pumpRed : Colour (0xff2c2a33));
        g.drawLine ({ gc.getPointOnCircumference (8.5f, a), gc.getPointOnCircumference (11.0f, a) }, 1.2f);
    }
    const float na = juce::jmap (gauge, -2.2f, 2.2f) + 0.08f * std::sin (time * 40.0f) * gauge;
    g.setColour (c2::pumpRed); g.drawLine ({ gc, gc.getPointOnCircumference (10.0f, na) }, 2.0f);
    g.setColour (Colour (0xff2c2a33)); g.fillEllipse (gc.x - 2, gc.y - 2, 4, 4);

    // T-handle
    auto h = Rectangle<float> (bx - 46, handleY - 7, 92, 14);
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (h.translated (0, 3), 7);
    g.setGradientFill (juce::ColourGradient (Colour (0xff3d3a47), 0, h.getY(), Colour (0xff1c1a22), 0, h.getBottom(), false));
    g.fillRoundedRectangle (h, 7);
    g.setColour (c::lime); g.fillRoundedRectangle (h.getX() + 3, h.getY() + 3, 16, 8, 4);
    g.fillRoundedRectangle (h.getRight() - 19, h.getY() + 3, 16, 8, 4);

    // "action lines" when slammed down
    if (handle > 0.6f)
    {
        g.setColour (juce::Colours::white.withAlpha ((handle - 0.6f) * 2.0f));
        for (int s = -1; s <= 1; s += 2)
            for (int k = 0; k < 3; ++k)
                g.drawLine (bx + s * (54.0f + k * 4), handleY - 10.0f + k * 9, bx + s * (64.0f + k * 4), handleY - 14.0f + k * 9, 2.0f);
    }
}

void Stage::drawDuck (juce::Graphics& g, juce::Point<float> gp, float sc, float sq, float strain)
{
    const float u = 72.0f * sc;
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::scale (1.0f + sq, 1.0f - sq, gp.x, gp.y));

    // shadow
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (gp.x - u * 1.0f, gp.y - u * 0.08f, u * 2.0f, u * 0.2f);

    const auto bodyC = juce::Point<float> (gp.x, gp.y - u * 0.55f);
    juce::Path body;
    body.addEllipse (bodyC.x - u, bodyC.y - u * 0.55f, u * 2.0f, u * 1.1f);
    juce::Path tail;
    tail.startNewSubPath (bodyC.x - u * 0.85f, bodyC.y - u * 0.05f);
    tail.quadraticTo (bodyC.x - u * 1.35f, bodyC.y - u * 0.75f, bodyC.x - u * 1.12f, bodyC.y - u * 0.82f);
    tail.quadraticTo (bodyC.x - u * 0.85f, bodyC.y - u * 0.75f, bodyC.x - u * 0.35f, bodyC.y - u * 0.38f);
    tail.closeSubPath();
    const auto headC = juce::Point<float> (gp.x + u * 0.62f, gp.y - u * 1.38f);
    juce::Path head; head.addEllipse (headC.x - u * 0.48f, headC.y - u * 0.48f, u * 0.96f, u * 0.96f);

    const float outline = juce::jmax (1.5f, u * 0.045f);
    for (auto* p : { &tail, &body, &head }) // back to front: fill + outline each part
    {
        g.setGradientFill (juce::ColourGradient (c2::duck.brighter (0.15f), bodyC.x - u * 0.6f, bodyC.y - u, c2::duckDark, bodyC.x + u * 0.6f, bodyC.y + u * 0.6f, false));
        g.fillPath (*p);
        g.setColour (c2::duckDark.darker (0.35f));
        g.strokePath (*p, juce::PathStrokeType (outline, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // inflatable seams + shine
    g.setColour (c2::duckDark.withAlpha (0.6f));
    juce::Path seam; seam.addCentredArc (bodyC.x, bodyC.y, u * 0.9f, u * 0.42f, 0, -1.2f, 1.2f, true);
    g.strokePath (seam, juce::PathStrokeType (outline * 0.6f));
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (bodyC.x - u * 0.6f, bodyC.y - u * 0.42f, u * 0.5f, u * 0.16f);
    g.fillEllipse (headC.x - u * 0.3f, headC.y - u * 0.36f, u * 0.22f, u * 0.12f);

    // wing
    juce::Path wing;
    wing.startNewSubPath (bodyC.x - u * 0.15f, bodyC.y - u * 0.15f);
    wing.quadraticTo (bodyC.x + u * 0.35f, bodyC.y + u * 0.35f, bodyC.x + u * 0.55f, bodyC.y - u * 0.05f);
    g.setColour (c2::duckDark.darker (0.2f)); g.strokePath (wing, juce::PathStrokeType (outline, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // beak
    const float bw = u * (0.52f + strain * 0.1f);
    juce::Path beak; beak.addRoundedRectangle (headC.x + u * 0.28f, headC.y - u * 0.02f, bw, u * 0.24f, u * 0.12f);
    g.setColour (c2::beak); g.fillPath (beak);
    g.setColour (c2::beak.darker (0.4f)); g.strokePath (beak, juce::PathStrokeType (outline * 0.8f));
    g.drawLine (headC.x + u * 0.32f, headC.y + u * 0.1f, headC.x + u * 0.28f + bw, headC.y + u * 0.1f, outline * 0.5f);

    // eye: widens as it gets over-inflated
    const float er = u * (0.12f + 0.09f * strain);
    const auto ec = juce::Point<float> (headC.x + u * 0.12f, headC.y - u * 0.12f);
    g.setColour (juce::Colours::white); g.fillEllipse (ec.x - er, ec.y - er, er * 2, er * 2);
    g.setColour (juce::Colours::black); g.drawEllipse (ec.x - er, ec.y - er, er * 2, er * 2, outline * 0.6f);
    const float pr = er * (0.55f - 0.25f * strain);
    g.fillEllipse (ec.x - pr + er * 0.2f, ec.y - pr, pr * 2, pr * 2);
    // cheek blush + sweat when it's about to blow
    g.setColour (c2::pumpRed.withAlpha (0.25f + 0.4f * strain));
    g.fillEllipse (headC.x + u * 0.02f, headC.y + u * 0.12f, u * 0.22f, u * 0.12f);
    if (strain > 0.6f)
    {
        const float a = (strain - 0.6f) * 2.5f;
        const float dy = std::fmod (time * 1.5f, 1.0f) * u * 0.3f;
        juce::Path drop;
        const float dx = headC.x - u * 0.45f, dy0 = headC.y - u * 0.5f + dy;
        drop.startNewSubPath (dx, dy0 - u * 0.12f);
        drop.quadraticTo (dx + u * 0.09f, dy0 + u * 0.02f, dx, dy0 + u * 0.05f);
        drop.quadraticTo (dx - u * 0.09f, dy0 + u * 0.02f, dx, dy0 - u * 0.12f);
        g.setColour (Colour (0xff7fd4ff).withAlpha (juce::jmin (1.0f, a))); g.fillPath (drop);
    }

    // valve
    const auto vp = juce::Point<float> (gp.x - u * 0.87f, gp.y - u * 0.5f);
    g.setColour (Colour (0xff2c2a33)); g.fillRoundedRectangle (vp.x - u * 0.12f, vp.y - u * 0.09f, u * 0.2f, u * 0.18f, u * 0.05f);
}

void Stage::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    // backdrop
    g.setColour (c::screen); g.fillRoundedRectangle (r, 14);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, 14); g.reduceClipRegion (clip);
        g.setGradientFill (juce::ColourGradient (c::violet.withAlpha (0.18f), r.getCentreX(), r.getY(), juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), false));
        g.fillRect (r);
        // checker floor
        const float ground = r.getBottom() - 22.0f;
        for (int i = 0; i < 40; ++i)
        {
            g.setColour ((i % 2) ? Colour (0xff17161d) : Colour (0xff1d1b24));
            g.fillRect (r.getX() + i * 16.0f, ground, 16.0f, 22.0f);
        }
        g.setColour (c::edge); g.drawHorizontalLine ((int) ground, r.getX(), r.getRight());

        drawPump (g, { r.getX() + 20, r.getY(), 160, ground - r.getY() });

        // hose from pump base to duck valve
        const auto start = juce::Point<float> (r.getX() + 20 + 70 + 14, ground - 10);
        const auto end = valvePos();
        hose.clear();
        hose.startNewSubPath (start);
        hose.cubicTo (start.x + 60, ground + 18, end.x - 90, ground + 10, end.x - 6, end.y);
        g.setColour (Colour (0xff111015)); g.strokePath (hose, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (Colour (0xff35323d)); g.strokePath (hose, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // air puffs travelling down the hose
        const float len = hose.getLength();
        for (auto& pf : puffs)
            if (pf.t > 0)
            {
                const auto pt = hose.getPointAlongPath (len * pf.t);
                const float rr = 3.0f + 4.0f * pf.size * std::sin (pf.t * juce::MathConstants<float>::pi);
                g.setColour (juce::Colours::white.withAlpha (0.85f * (1.0f - pf.t * 0.5f)));
                g.fillEllipse (pt.x - rr, pt.y - rr, rr * 2, rr * 2);
            }

        // the duck
        const auto gp = juce::Point<float> (duckX, ground);
        if (! isPopping())
        {
            const float sc = (0.45f + 0.55f * inflate) * regrow;
            const float wob = 0.012f * inflate * std::sin (time * 9.0f);
            drawDuck (g, gp, sc, juce::jlimit (-0.25f, 0.25f, squash * 0.12f + wob), inflate);
        }
        else
        {
            // POP! starburst
            const float a = juce::jlimit (0.0f, 1.0f, 1.0f - popAge * 1.2f);
            const auto ctr = juce::Point<float> (gp.x + 20, gp.y - 70);
            const float rad = 50.0f + 50.0f * std::min (1.0f, popAge * 6.0f);
            juce::Path star; star.addStar (ctr, 14, rad * 0.55f, rad, 0.2f);
            g.setColour (Colour (0xffffffff).withAlpha (a)); g.fillPath (star);
            g.setColour (c2::pumpRed.withAlpha (a)); g.strokePath (star, juce::PathStrokeType (4.0f));
            juce::GlyphArrangement ga; ga.addLineOfText (logoF (44.0f * (0.8f + 0.4f * std::min (1.0f, popAge * 6.0f))), "POP!", 0, 0);
            juce::Path tp; ga.createPath (tp);
            auto tb = tp.getBounds();
            tp.applyTransform (juce::AffineTransform::translation (ctr.x - tb.getCentreX(), ctr.y - tb.getCentreY()).rotated (-0.12f, ctr.x, ctr.y));
            g.setColour (c2::pumpRed.withAlpha (a)); g.fillPath (tp);
            g.setColour (juce::Colours::black.withAlpha (a)); g.strokePath (tp, juce::PathStrokeType (2.0f));
        }
        for (auto& b : bits)
        {
            juce::Graphics::ScopedSaveState s2 (g);
            g.addTransform (juce::AffineTransform::rotation (b.rot).translated (b.pos));
            g.setColour (b.col.withAlpha (juce::jlimit (0.0f, 1.0f, b.life)));
            g.fillRect (-5.0f, -3.0f, 10.0f, 6.0f);
        }

        // counter
        g.setFont (monoF (11.0f));
        g.setColour (c::dim);
        g.drawText ("PUMPS", r.reduced (16, 10).removeFromTop (16), juce::Justification::topRight);
        auto dots = Rectangle<float> (r.getRight() - 16 - strokesToPop * 14.0f, r.getY() + 30, strokesToPop * 14.0f, 10);
        for (int i = 0; i < strokesToPop; ++i)
        {
            g.setColour (i < strokes ? (i >= strokesToPop - 2 ? c2::pumpRed : c::lime) : c::edge);
            g.fillEllipse (dots.getX() + i * 14.0f + 2, dots.getY(), 9, 9);
        }
        g.setColour (c::dim.withAlpha (0.6f)); g.setFont (monoF (9.5f));
        g.drawText ("click the duck to pop it", r.reduced (16, 8).removeFromBottom (14).withTrimmedBottom (-2), juce::Justification::bottomRight);
    }
    g.setColour (c::edge); g.drawRoundedRectangle (r.reduced (0.5f), 14, 1);
}

// ---------------------------------------------------------------------------
PumpEditor::PumpEditor (PumpProcessor& p) : AudioProcessorEditor (p), proc (p), stage (p), display (p), mode (p.apvts)
{
    gLnf = &lnf;
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    setLookAndFeel (&lnf);
    addAndMakeVisible (stage);
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
    setSize (640, 600);
    startTimerHz (60);
}

PumpEditor::~PumpEditor() { stopTimer(); juce::LookAndFeel::setDefaultLookAndFeel (nullptr); setLookAndFeel (nullptr); }

void PumpEditor::timerCallback()
{
    display.scLevel = proc.uiScLevelDb.load() > display.scLevel ? proc.uiScLevelDb.load() : display.scLevel - 1.5f;
    display.repaint();
    stage.tick (1.0f / 60.0f);
    stage.repaint();
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
    stage.setBounds (20, 66, getWidth() - 40, 190);
    display.setBounds (20, 266, getWidth() - 40, 180);
    const int kw = (getWidth() - 40) / (int) knobs.size();
    for (size_t i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (20 + (int) i * kw, 460, kw, 122);
}

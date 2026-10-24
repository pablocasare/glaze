#include "PluginEditor.h"
#include "BinaryData.h"

using namespace lui;
using juce::Colour;
using juce::Rectangle;
using juce::Point;

namespace c
{
    const Colour bg      { 0xffe9eef7 };
    const Colour panel   { 0xfff7f9fd };
    const Colour card    { 0xffffffff };
    const Colour edge    { 0xffd5ddeb };
    const Colour blue    { 0xff2f7cf6 };
    const Colour blueLt  { 0xffa9c9ff };
    const Colour navy    { 0xff1c2540 };
    const Colour text    { 0xff1d2433 };
    const Colour dim     { 0xff8390a8 };
    const Colour orange  { 0xffff7a1a };
    const Colour yellow  { 0xffffd23f };
    const Colour red     { 0xffff4d5e };
    const Colour pink    { 0xffff9db0 };
}

static LnF* gL = nullptr;
static juce::Font mono (float h) { return juce::Font (juce::FontOptions().withTypeface (gL->tfMono).withHeight (h)); }
static juce::Font logo (float h) { return juce::Font (juce::FontOptions().withTypeface (gL->tfLogo).withHeight (h)); }

static juce::Path textPath (const juce::String& s, juce::Font f)
{
    juce::GlyphArrangement ga; ga.addLineOfText (f, s, 0, 0);
    juce::Path p; ga.createPath (p); return p;
}

// ---------------------------------------------------------------------------
LnF::LnF()
{
    tfLogo = juce::Typeface::createSystemTypefaceFor (BinaryData::RighteousRegular_ttf, BinaryData::RighteousRegular_ttfSize);
    tfMono = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    setColour (juce::PopupMenu::backgroundColourId, c::card);
    setColour (juce::PopupMenu::textColourId, c::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, c::blue.withAlpha (0.15f));
    setColour (juce::PopupMenu::highlightedTextColourId, c::blue);
    setColour (juce::ComboBox::textColourId, c::text);
    setColour (juce::Slider::textBoxTextColourId, c::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, c::card);
    setColour (juce::TextEditor::textColourId, c::text);
    setColour (juce::TextEditor::highlightColourId, c::blue.withAlpha (0.25f));
    setColour (juce::CaretComponent::caretColourId, c::blue);
    setColour (juce::Label::textColourId, c::text);
}

void LnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3);
    const float rad = std::min (r.getWidth(), r.getHeight()) * 0.5f;
    const auto ctr = r.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const bool en = s.isEnabled();
    juce::Path track; track.addCentredArc (ctr.x, ctr.y, rad * 0.88f, rad * 0.88f, 0, a0, a1, true);
    g.setColour (c::edge); g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc (ctr.x, ctr.y, rad * 0.88f, rad * 0.88f, 0, a0, ang, true);
    g.setColour (en ? c::blue : c::dim.withAlpha (0.5f));
    g.strokePath (val, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float br = rad * 0.64f;
    g.setColour (juce::Colours::black.withAlpha (0.12f)); g.fillEllipse (ctr.x - br, ctr.y - br + 3, br * 2, br * 2);
    g.setGradientFill (juce::ColourGradient (Colour (0xff3a4258), ctr.x, ctr.y - br, Colour (0xff141a2b), ctr.x, ctr.y + br, false));
    g.fillEllipse (ctr.x - br, ctr.y - br, br * 2, br * 2);
    g.setColour (juce::Colours::white.withAlpha (0.15f)); g.drawEllipse (ctr.x - br + 1, ctr.y - br + 1, br * 2 - 2, br * 2 - 2, 1);
    g.setColour (juce::Colours::white);
    g.drawLine ({ ctr.getPointOnCircumference (br * 0.35f, ang), ctr.getPointOnCircumference (br * 0.82f, ang) }, 2.4f);
}

void LnF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& b)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (c::card); g.fillRoundedRectangle (r, 6);
    g.setColour (b.isMouseOver() ? c::blue : c::edge); g.drawRoundedRectangle (r, 6, 1);
    juce::Path a; const float ax = (float) w - 13, ay = (float) h * 0.5f;
    a.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (c::blue); g.fillPath (a);
}
juce::Font LnF::getComboBoxFont (juce::ComboBox&) { return mono (12.0f); }
juce::Font LnF::getPopupMenuFont() { return mono (13.0f); }
void LnF::positionComboBoxText (juce::ComboBox& b, juce::Label& l) { l.setBounds (8, 0, b.getWidth() - 24, b.getHeight()); l.setFont (getComboBoxFont (b)); }
juce::Label* LnF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (11.5f)); l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::backgroundColourId, Colour (0xffeef3fb));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

// ---------------------------------------------------------------------------
Card::Card (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& on, const juce::String& nm)
    : s (st), onId (on), name (nm)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    slider.setMouseDragSensitivity (200);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, slider);
    auto* p = st.getParameter (id);
    slider.textFromValueFunction = [p] (double v)
    { return p->getLabel() == "dB" ? juce::String (v, 1) + " dB" : juce::String (juce::roundToInt (v)) + "%"; };
    slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.updateText();
}
void Card::resized() { slider.setBounds (getLocalBounds().reduced (6).withTrimmedTop (26).withTrimmedBottom (4)); }
void Card::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (juce::Colours::black.withAlpha (0.05f)); g.fillRoundedRectangle (r.translated (0, 2), 12);
    g.setColour (c::card); g.fillRoundedRectangle (r, 12);
    g.setColour (c::edge); g.drawRoundedRectangle (r, 12, 1);
    const bool on = onId.isEmpty() || s.getRawParameterValue (onId)->load() > 0.5f;
    auto top = r.removeFromTop (26).reduced (8, 0);
    if (onId.isNotEmpty())
    {
        auto dot = top.removeFromLeft (12).withSizeKeepingCentre (11, 11);
        g.setColour (on ? c::blue : c::edge); g.fillEllipse (dot);
        if (on) { g.setColour (juce::Colours::white); g.fillEllipse (dot.reduced (3.5f)); }
        top.removeFromLeft (4);
    }
    g.setFont (mono (10.5f));
    g.setColour (on ? c::text : c::dim);
    g.drawText (name, top, juce::Justification::centredLeft);
    slider.setEnabled (on);
}
void Card::mouseDown (const juce::MouseEvent& e)
{
    if (onId.isEmpty() || e.y > 26) return;
    auto* p = s.getParameter (onId);
    p->beginChangeGesture(); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture();
    repaint();
}

// ---------------------------------------------------------------------------
// Rocket dial
juce::String RocketDial::status() const
{
    if (blasting()) return "DROP!!";
    if (b < 0.05f) return "chillin'";
    if (b < 0.3f)  return "warming up";
    if (b < 0.65f) return "it's building...";
    if (b < 0.95f) return "here it comes";
    return "LIFTOFF!!";
}

void RocketDial::tick (float dt)
{
    time += dt;
    const float nb = proc.uiBuild.load();
    // a fast fall from high build to ~0 = the drop -> blast off
    recentMax = std::max (nb, recentMax - dt * 1.2f);
    if (! blasting() && nb < 0.08f && recentMax > 0.55f && b > nb + 0.02f) { blastAge = 0; blastV = 120; blastY = hover; recentMax = 0; }
    b = nb;

    hover += ((b > 0.95f ? 12.0f + 3.0f * std::sin (time * 6.0f) : 0.0f) - hover) * std::min (1.0f, dt * 4.0f);

    if (blasting())
    {
        blastAge += dt;
        blastV += 1400.0f * dt;
        blastY += blastV * dt;
        if (blastAge > 1.6f) { blastAge = -1; blastY = 0; landing = 1; }
    }
    landing = std::max (0.0f, landing - dt * 1.4f);

    // smoke
    const float cx = getWidth() * 0.5f, padY = 130.0f + 104.0f * 0.62f;
    const float rate = blasting() ? 60.0f : b * b * 70.0f;
    int n = (int) (rate * dt + rng.nextFloat());
    for (int i = 0; i < n; ++i)
    {
        const float dir = rng.nextBool() ? 1.0f : -1.0f;
        smoke.push_back ({ { cx + dir * rng.nextFloat() * 8.0f, padY - 2 }, { dir * (40.0f + rng.nextFloat() * 80.0f), -10.0f - rng.nextFloat() * 25.0f },
                           4.0f + rng.nextFloat() * 4.0f, 1.0f });
    }
    for (auto& s : smoke) { s.pos += s.vel * dt; s.vel *= (1.0f - dt * 1.2f); s.r += dt * 22.0f; s.life -= dt * 0.9f; }
    smoke.erase (std::remove_if (smoke.begin(), smoke.end(), [] (const Puff& s) { return s.life <= 0; }), smoke.end());
}

void RocketDial::drawFace (juce::Graphics& g, Point<float> ctr, float r, float bb)
{
    g.setColour (c::navy); g.fillEllipse (ctr.x - r, ctr.y - r, r * 2, r * 2);
    g.setColour (juce::Colours::white.withAlpha (0.18f)); g.fillEllipse (ctr.x - r * 0.65f, ctr.y - r * 0.8f, r * 0.7f, r * 0.4f);
    const float u = r / 15.0f;
    const auto eL = ctr + Point<float> (-5.5f * u, -2.5f * u), eR = ctr + Point<float> (5.5f * u, -2.5f * u);
    const juce::PathStrokeType st (1.6f * u, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (c::pink.withAlpha (0.35f + 0.5f * bb));
    g.fillEllipse (ctr.x - 10.5f * u, ctr.y + 1.5f * u, 4.5f * u, 2.6f * u);
    g.fillEllipse (ctr.x + 6.0f * u, ctr.y + 1.5f * u, 4.5f * u, 2.6f * u);
    g.setColour (juce::Colours::white);

    if (blasting())
    {   // > <  squint + huge grin
        for (float s : { -1.0f, 1.0f })
        {
            const auto e = s < 0 ? eL : eR;
            juce::Path p; p.startNewSubPath (e.x - 2.2f * u * s, e.y - 2 * u); p.lineTo (e.x + 1.5f * u * s, e.y); p.lineTo (e.x - 2.2f * u * s, e.y + 2 * u);
            g.strokePath (p, st);
        }
        juce::Path m; m.addPieSegment (ctr.x - 6 * u, ctr.y + 0.5f * u, 12 * u, 9 * u, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi * 3, 0);
        g.fillPath (m);
        return;
    }
    if (bb < 0.25f)
    {   // sleepy happy arcs
        for (auto e : { eL, eR }) { juce::Path p; p.addCentredArc (e.x, e.y + 0.5f * u, 2.6f * u, 2.0f * u, 0, -1.3f, 1.3f, true); g.strokePath (p, st); }
        juce::Path m; m.addCentredArc (ctr.x, ctr.y + 3.5f * u, 3.0f * u, 2.0f * u, 0, juce::MathConstants<float>::pi * 0.65f, juce::MathConstants<float>::pi * 1.35f, true);
        g.strokePath (m, st);
        return;
    }
    const float er = (bb < 0.6f ? 2.4f : (bb < 0.9f ? 3.2f : 4.2f)) * u;
    const float pr = (bb < 0.9f ? 0.55f : 0.32f) * er;
    for (auto e : { eL, eR })
    {
        g.setColour (juce::Colours::white); g.fillEllipse (e.x - er, e.y - er, er * 2, er * 2);
        g.setColour (c::navy); g.fillEllipse (e.x - pr, e.y - pr + 0.3f * u, pr * 2, pr * 2);
        g.setColour (juce::Colours::white); g.fillEllipse (e.x - pr * 0.3f + pr * 0.3f, e.y - pr * 0.6f, pr * 0.6f, pr * 0.6f);
    }
    g.setColour (juce::Colours::white);
    if (bb < 0.6f)
    {
        juce::Path m; m.addCentredArc (ctr.x, ctr.y + 3.0f * u, 4.0f * u, 3.0f * u, 0, juce::MathConstants<float>::pi * 0.6f, juce::MathConstants<float>::pi * 1.4f, true);
        g.strokePath (m, st);
    }
    else if (bb < 0.9f)
    {   // open grin
        juce::Path m; m.addPieSegment (ctr.x - 5 * u, ctr.y + 2.0f * u, 10 * u, 7 * u, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi * 3, 0);
        g.fillPath (m);
        g.setColour (c::pink); g.fillEllipse (ctr.x - 2.2f * u, ctr.y + 6.0f * u, 4.4f * u, 2.2f * u);
    }
    else
    {   // screaming O
        const float mw = 5.0f * u * (1.0f + 0.08f * std::sin (time * 30.0f));
        g.fillEllipse (ctr.x - mw * 0.5f, ctr.y + 2.5f * u, mw, 6.5f * u);
        g.setColour (c::navy); g.fillEllipse (ctr.x - mw * 0.32f, ctr.y + 3.6f * u, mw * 0.64f, 4.3f * u);
    }
}

void RocketDial::drawRocket (juce::Graphics& g, Point<float> base, float k, float bb, float flameLen)
{
    auto T = juce::AffineTransform::scale (k).translated (base);
    // flame
    if (flameLen > 0.5f)
    {
        const float fl = flameLen * (0.85f + 0.15f * std::sin (time * 47.0f) + 0.08f * rng.nextFloat());
        juce::Path outer;
        outer.startNewSubPath (-11, -14);
        outer.quadraticTo (-14, -14 + fl * 0.45f, 0, -14 + fl);
        outer.quadraticTo (14, -14 + fl * 0.45f, 11, -14);
        outer.closeSubPath();
        juce::Path inner;
        inner.startNewSubPath (-6, -14);
        inner.quadraticTo (-7, -14 + fl * 0.35f, 0, -14 + fl * 0.65f);
        inner.quadraticTo (7, -14 + fl * 0.35f, 6, -14);
        inner.closeSubPath();
        g.setColour (c::orange.withAlpha (0.35f)); g.fillPath (outer, juce::AffineTransform::scale (1.5f, 1.15f).followedBy (T));
        g.setColour (c::orange); g.fillPath (outer, T);
        g.setColour (c::yellow); g.fillPath (inner, T);
    }
    const Colour ink = c::navy;
    const juce::PathStrokeType st (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    // fins
    juce::Path finL; finL.startNewSubPath (-20, -50); finL.quadraticTo (-38, -36, -40, -8); finL.lineTo (-21, -20); finL.closeSubPath();
    juce::Path finR; finR.startNewSubPath (20, -50); finR.quadraticTo (38, -36, 40, -8); finR.lineTo (21, -20); finR.closeSubPath();
    for (auto* f : { &finL, &finR }) { g.setColour (c::red); g.fillPath (*f, T); g.setColour (ink); g.strokePath (*f, st, T); }
    // nozzle
    juce::Path noz; noz.startNewSubPath (-12, -22); noz.lineTo (12, -22); noz.lineTo (15, -13); noz.lineTo (-15, -13); noz.closeSubPath();
    g.setColour (Colour (0xff6b7489)); g.fillPath (noz, T); g.setColour (ink); g.strokePath (noz, st, T);
    // body
    juce::Path body;
    body.startNewSubPath (-22, -22);
    body.lineTo (-22, -78);
    body.quadraticTo (-20, -112, 0, -128);
    body.quadraticTo (20, -112, 22, -78);
    body.lineTo (22, -22);
    body.quadraticTo (0, -16, -22, -22);
    body.closeSubPath();
    {
        juce::Path tb = body; tb.applyTransform (T);
        auto bb2 = tb.getBounds();
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, bb2.getX(), 0, Colour (0xffc9d3e6), bb2.getRight(), 0, false));
        g.fillPath (tb);
        // nose cone
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (Rectangle<int> (0, 0, getWidth(), (int) (base.y - 96 * k)));
        g.setColour (c::red); g.fillPath (tb);
    }
    // stripe
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path tb = body; tb.applyTransform (T); g.reduceClipRegion (tb);
        g.setColour (c::blue);
        g.fillRect (Rectangle<float> (base.x - 30 * k, base.y - 38 * k, 60 * k, 7 * k));
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.fillRect (Rectangle<float> (base.x - 15 * k, base.y - 110 * k, 5 * k, 80 * k));
    }
    g.setColour (ink); g.strokePath (body, st, T);
    // porthole with face
    const auto pc = Point<float> (0, -64).transformedBy (T);
    const float pr = 16.0f * k;
    g.setColour (Colour (0xffb8c2d6)); g.fillEllipse (pc.x - pr - 3 * k, pc.y - pr - 3 * k, (pr + 3 * k) * 2, (pr + 3 * k) * 2);
    g.setColour (ink); g.drawEllipse (pc.x - pr - 3 * k, pc.y - pr - 3 * k, (pr + 3 * k) * 2, (pr + 3 * k) * 2, 2.0f);
    drawFace (g, pc, pr, bb);
    // sweat when maxed
    if (bb > 0.9f && ! blasting())
    {
        const float dy = std::fmod (time * 1.8f, 1.0f) * 14.0f;
        juce::Path d; const float dx = pc.x + pr + 8, d0 = pc.y - pr + dy;
        d.startNewSubPath (dx, d0 - 5); d.quadraticTo (dx + 4, d0 + 1, dx, d0 + 3); d.quadraticTo (dx - 4, d0 + 1, dx, d0 - 5);
        g.setColour (Colour (0xff6fc3ff)); g.fillPath (d);
    }
}

void RocketDial::paint (juce::Graphics& g)
{
    const float cx = getWidth() * 0.5f, cy = 130.0f, R = 104.0f;
    const float bb = b;

    // ring
    const float a0 = juce::degreesToRadians (225.0f), a1 = juce::degreesToRadians (495.0f);
    juce::Path track; track.addCentredArc (cx, cy, R + 13, R + 13, 0, a0, a1, true);
    g.setColour (c::edge); g.strokePath (track, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (bb > 0.002f)
    {
        juce::Path val; val.addCentredArc (cx, cy, R + 13, R + 13, 0, a0, a0 + bb * (a1 - a0), true);
        g.setGradientFill (juce::ColourGradient (c::blue, cx - R, cy + R, c::orange, cx + R, cy + R, false));
        g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const auto hp = Point<float> (cx, cy).getPointOnCircumference (R + 13, a0 + bb * (a1 - a0));
    g.setColour (juce::Colours::white); g.fillEllipse (hp.x - 8, hp.y - 8, 16, 16);
    g.setColour (bb > 0.6f ? c::orange : c::blue); g.drawEllipse (hp.x - 8, hp.y - 8, 16, 16, 3);
    // tick marks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + i / 10.0f * (a1 - a0);
        g.setColour (c::dim.withAlpha (0.5f));
        g.drawLine ({ Point<float> (cx, cy).getPointOnCircumference (R + 22, a), Point<float> (cx, cy).getPointOnCircumference (R + 26, a) }, 1.5f);
    }

    // scene inside the circle
    juce::Path circle; circle.addEllipse (cx - R, cy - R, R * 2, R * 2);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (circle);
        const Colour skyTop = Colour (0xffbfe0ff).interpolatedWith (Colour (0xff20265a), juce::jmin (1.0f, bb * 1.2f));
        const Colour skyBot = Colour (0xffeaf4ff).interpolatedWith (Colour (0xffff9a5a), juce::jmin (1.0f, bb * 1.1f));
        g.setGradientFill (juce::ColourGradient (skyTop, cx, cy - R, skyBot, cx, cy + R, false));
        g.fillRect (circle.getBounds());
        // stars fade in as it builds
        juce::Random sr (7);
        for (int i = 0; i < 26; ++i)
        {
            const float sx = cx - R + sr.nextFloat() * R * 2, sy = cy - R + sr.nextFloat() * R * 1.1f;
            const float tw = 0.6f + 0.4f * std::sin (time * 3.0f + i);
            g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, (bb - 0.35f) * 2.0f) * tw));
            g.fillEllipse (sx, sy, 2.2f, 2.2f);
        }
        // ground + pad
        const float padY = cy + R * 0.62f;
        g.setColour (Colour (0xff9fb0c9)); g.fillRect (cx - R, padY + 6, R * 2, R);
        g.setColour (Colour (0xff6b7489)); g.fillRoundedRectangle (cx - 34, padY, 68, 9, 3);
        g.setColour (Colour (0xff4a5266)); g.fillRect (cx - 28, padY + 9, 5.0f, 14.0f); g.fillRect (cx + 23, padY + 9, 5.0f, 14.0f);

        // smoke behind rocket
        for (auto& s : smoke)
        {
            g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 0.85f, s.life)));
            g.fillEllipse (s.pos.x - s.r, s.pos.y - s.r, s.r * 2, s.r * 2);
        }

        // rocket
        const float shake = (blasting() ? 2.0f : bb * bb * 3.2f);
        const float k = 0.9f;
        float lift = hover + (blasting() ? blastY : 0.0f);
        if (landing > 0) lift = landing * landing * 260.0f; // drifting back down after a drop
        const Point<float> base (cx + (rng.nextFloat() - 0.5f) * shake * 2, padY + 7.0f * k - lift + (rng.nextFloat() - 0.5f) * shake);
        const float flame = blasting() ? 95.0f : (landing > 0 ? 25.0f : bb * 58.0f);
        drawRocket (g, base, k, bb, flame);
    }
    g.setColour (c::navy.withAlpha (0.8f)); g.drawEllipse (cx - R, cy - R, R * 2, R * 2, 2.5f);

    // DROP!! burst
    if (blasting() && blastAge < 1.0f)
    {
        const float a = juce::jlimit (0.0f, 1.0f, 1.2f - blastAge * 1.2f);
        const float s = 0.7f + 0.5f * std::min (1.0f, blastAge * 6.0f);
        auto tp = textPath ("DROP!!", logo (46.0f * s));
        auto tb = tp.getBounds();
        tp.applyTransform (juce::AffineTransform::translation (cx - tb.getCentreX(), cy - 10 - tb.getCentreY()).rotated (-0.1f, cx, cy));
        g.setColour (juce::Colours::white.withAlpha (a)); g.strokePath (tp, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c::orange.withAlpha (a)); g.fillPath (tp);
        g.setColour (c::navy.withAlpha (a)); g.strokePath (tp, juce::PathStrokeType (1.5f));
    }

    // value readout
    auto lbl = Rectangle<float> (0, cy + R + 30, (float) getWidth(), 26);
    g.setFont (logo (24.0f));
    g.setColour (c::text);
    g.drawText ("build " + juce::String (juce::roundToInt (bb * 100)) + "%", lbl, juce::Justification::centred);
    g.setFont (mono (9.5f)); g.setColour (c::dim);
    g.drawText ("drag  |  right-click = automate", lbl.translated (0, 26).withHeight (14), juce::Justification::centred);
}

void RocketDial::mouseDown (const juce::MouseEvent& e)
{
    auto* p = proc.apvts.getParameter ("build");
    if (e.mods.isPopupMenu())
    {
        // the DAW's own parameter menu (FL: "Create automation clip", Ableton/Logic: automate, MIDI learn...)
        if (auto* ed = findParentComponentOfClass<juce::AudioProcessorEditor>())
            if (auto* ctx = ed->getHostContext())
                if (auto menu = ctx->getContextMenuForParameter (p))
                {
                    menu->showNativeMenu (ed->getLocalPoint (this, e.getPosition()));
                    return;
                }
        return;
    }
    startDragValue = p->convertFrom0to1 (p->getValue());
    p->beginChangeGesture();
}
void RocketDial::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    auto* p = proc.apvts.getParameter ("build");
    const float v = juce::jlimit (0.0f, 100.0f, startDragValue - (float) e.getDistanceFromDragStartY() * 0.5f + (float) e.getDistanceFromDragStartX() * 0.15f);
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}
void RocketDial::mouseUp (const juce::MouseEvent& e) { if (! e.mods.isPopupMenu()) proc.apvts.getParameter ("build")->endChangeGesture(); }
void RocketDial::mouseDoubleClick (const juce::MouseEvent&)
{
    auto* p = proc.apvts.getParameter ("build");
    p->beginChangeGesture(); p->setValueNotifyingHost (0.0f); p->endChangeGesture();
}

// ---------------------------------------------------------------------------
void Graph::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (c::card); g.fillRoundedRectangle (r, 12);
    g.setColour (c::edge); g.drawRoundedRectangle (r, 12, 1);
    auto plot = r.reduced (12, 10).withTrimmedBottom (18);
    const float fc = proc.uiCutoff.load(), q = proc.uiQ.load(), b = proc.uiBuild.load();
    auto xOf = [&] (float f) { return plot.getX() + plot.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
    auto yOf = [&] (float db) { return juce::jmap (juce::jlimit (-36.0f, 18.0f, db), -36.0f, 18.0f, plot.getBottom(), plot.getY()); };
    for (float f : { 100.f, 1000.f, 10000.f }) { g.setColour (c::edge.withAlpha (0.6f)); g.drawVerticalLine ((int) xOf (f), plot.getY(), plot.getBottom()); }
    juce::Path fill, line;
    for (float x = plot.getX(); x <= plot.getRight(); x += 2)
    {
        const float f = 20.0f * std::pow (1000.0f, (x - plot.getX()) / plot.getWidth());
        const float w = f / fc;
        const float mag = (w * w) / std::sqrt ((1 - w * w) * (1 - w * w) + (w / q) * (w / q));
        const float y = yOf (juce::Decibels::gainToDecibels (mag, -60.0f));
        if (line.isEmpty()) { line.startNewSubPath (x, y); fill.startNewSubPath (x, plot.getBottom()); } else line.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (plot.getRight(), plot.getBottom()); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c::blue.withAlpha (0.55f), 0, plot.getY(), c::blueLt.withAlpha (0.25f), 0, plot.getBottom(), false));
    g.fillPath (fill);
    g.setColour (c::blue); g.strokePath (line, juce::PathStrokeType (2.0f));
    const float dx = xOf (fc);
    g.setColour (c::navy); g.fillRoundedRectangle (dx - 5, yOf (0) - 5, 10, 10, 2);

    auto cap = r.reduced (12, 6).removeFromBottom (14);
    g.setFont (mono (10.5f)); g.setColour (c::dim);
    g.drawText ("low cut " + (fc >= 1000 ? juce::String (fc / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (fc))) + " Hz", cap, juce::Justification::centredLeft);
    auto amt = [&] (const char* id, const char* on) { return proc.apvts.getRawParameterValue (on)->load() > 0.5f ? juce::roundToInt (b * proc.apvts.getRawParameterValue (id)->load()) : 0; };
    g.drawText ("delay " + juce::String (amt ("delay", "delayOn")) + "%   reverb " + juce::String (amt ("reverb", "reverbOn"))
                    + "%   noise " + juce::String (amt ("noise", "noiseOn")) + "%", cap, juce::Justification::centredRight);
}

// ---------------------------------------------------------------------------
void HoldButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (held ? c::orange : c::navy); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (juce::Colours::white); g.setFont (mono (11.0f));
    g.drawText (held ? "BUILDING... LET GO TO DROP" : "HOLD TO BUILD", r, juce::Justification::centred);
}
void HoldButton::mouseDown (const juce::MouseEvent&) { held = true; proc.apvts.getParameter ("build")->beginChangeGesture(); repaint(); }
void HoldButton::mouseUp (const juce::MouseEvent&)
{
    held = false;
    auto* p = proc.apvts.getParameter ("build");
    p->setValueNotifyingHost (0.0f); p->endChangeGesture();
    repaint();
}
void HoldButton::tick (float dt)
{
    if (! held) return;
    auto* p = proc.apvts.getParameter ("build");
    p->setValueNotifyingHost (juce::jmin (1.0f, p->getValue() + dt / 4.0f)); // full build in ~4 s
}


// ---------------------------------------------------------------------------
AutoRow::AutoRow (LiftoffProcessor& p) : proc (p)
{
    len.addItemList (liftoff::lenNames, 1);
    lenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "autoLen", len);
    addAndMakeVisible (len);
}
int AutoRow::bar() const { return juce::roundToInt (proc.apvts.getRawParameterValue ("dropBar")->load()); }
void AutoRow::setBar (int b)
{
    auto* prm = proc.apvts.getParameter ("dropBar");
    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (2, 999, b)));
    repaint();
}
void AutoRow::resized()
{
    auto r = getLocalBounds().toFloat().reduced (10, 6);
    togR = r.removeFromLeft (128); r.removeFromLeft (14);
    r.removeFromLeft (84); // "drop at bar" label
    minusR = r.removeFromLeft (26); numR = r.removeFromLeft (52); plusR = r.removeFromLeft (26);
    r.removeFromLeft (16);
    r.removeFromLeft (80); // "build over" label
    len.setBounds (r.removeFromLeft (100).toNearestInt());
    r.removeFromLeft (14);
    statusR = r;
}
void AutoRow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (c::card); g.fillRoundedRectangle (r, 12);
    g.setColour (c::edge); g.drawRoundedRectangle (r, 12, 1);
    const bool on = proc.apvts.getRawParameterValue ("autoOn")->load() > 0.5f;
    // toggle
    g.setColour (on ? c::blue : c::bg); g.fillRoundedRectangle (togR, togR.getHeight() * 0.5f);
    g.setColour (on ? c::blue : c::edge); g.drawRoundedRectangle (togR, togR.getHeight() * 0.5f, 1);
    g.setColour (on ? juce::Colours::white : c::dim); g.setFont (mono (11.0f));
    g.drawText (on ? "AUTO BUILD: ON" : "AUTO BUILD: OFF", togR, juce::Justification::centred);
    // drop bar
    g.setColour (on ? c::text : c::dim); g.setFont (mono (10.5f));
    g.drawText ("drop at bar", Rectangle<float> (minusR.getX() - 84, minusR.getY(), 80, minusR.getHeight()), juce::Justification::centredRight);
    g.drawText ("build over", Rectangle<float> ((float) len.getX() - 80, minusR.getY(), 76, minusR.getHeight()), juce::Justification::centredRight);
    g.setColour (c::bg); g.fillRoundedRectangle (minusR.getUnion (plusR), 6);
    g.setColour (c::edge); g.drawRoundedRectangle (minusR.getUnion (plusR), 6, 1);
    g.setColour (c::blue); g.setFont (mono (15.0f));
    g.drawText ("-", minusR, juce::Justification::centred); g.drawText ("+", plusR, juce::Justification::centred);
    g.setColour (c::text); g.setFont (mono (13.0f));
    g.drawText (juce::String (bar()), numR, juce::Justification::centred);
    // live status
    juce::String st;
    const float b = proc.uiBar.load(), a = proc.uiAuto.load();
    const int lenBars = (int) liftoff::lenBars[juce::jlimit (0, 4, (int) proc.apvts.getRawParameterValue ("autoLen")->load())];
    const int start = bar() - lenBars;
    if (! on) st = "turn on: builds bars " + juce::String (juce::jmax (1, start)) + "-" + juce::String (bar()) + ", drops on " + juce::String (bar());
    else if (b < 0) st = "press play";
    else if (a > 0) st = "bar " + juce::String (b, 1) + "  |  building " + juce::String (juce::roundToInt (a * 100)) + "%";
    else if (b < start) st = "bar " + juce::String (b, 1) + "  |  builds from bar " + juce::String (start);
    else st = "bar " + juce::String (b, 1) + "  |  dropped!";
    g.setColour (on ? (a > 0 ? c::orange : c::text) : c::dim); g.setFont (mono (10.5f));
    g.drawFittedText (st, statusR.toNearestInt(), juce::Justification::centredLeft, 2);
}
void AutoRow::mouseDown (const juce::MouseEvent& e)
{
    draggingNum = false;
    if (togR.contains (e.position))
    {
        auto* prm = proc.apvts.getParameter ("autoOn");
        prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f); prm->endChangeGesture();
        repaint();
    }
    else if (minusR.contains (e.position)) setBar (bar() - (e.mods.isShiftDown() ? 8 : 1));
    else if (plusR.contains (e.position))  setBar (bar() + (e.mods.isShiftDown() ? 8 : 1));
    else if (numR.contains (e.position)) { draggingNum = true; dragStartBar = bar(); }
}
void AutoRow::mouseDrag (const juce::MouseEvent& e) { if (draggingNum) setBar (dragStartBar - e.getDistanceFromDragStartY() / 6); }
void AutoRow::mouseUp (const juce::MouseEvent&) { draggingNum = false; }

// ---------------------------------------------------------------------------
LiftoffEditor::LiftoffEditor (LiftoffProcessor& p) : AudioProcessorEditor (p), proc (p), dial (p), graph (p), hold (p), autoRow (p)
{
    gL = &lnf;
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    setLookAndFeel (&lnf);
    addAndMakeVisible (dial); addAndMakeVisible (graph); addAndMakeVisible (hold); addAndMakeVisible (autoRow);
    delayTime.addItemList (liftoff::delayNames, 1);
    delayAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "delayTime", delayTime);
    addAndMakeVisible (delayTime);
    const char* specs[][3] { { "filter", "filterOn", "filter" }, { "resonance", "resOn", "reso" }, { "delay", "delayOn", "delay" },
                             { "reverb", "reverbOn", "reverb" }, { "noise", "noiseOn", "noise" }, { "output", "", "output" } };
    for (auto& s : specs) { cards.push_back (std::make_unique<lui::Card> (p.apvts, s[0], s[1], s[2])); addAndMakeVisible (*cards.back()); }
    setSize (800, 504);
    startTimerHz (60);
}
LiftoffEditor::~LiftoffEditor() { stopTimer(); juce::LookAndFeel::setDefaultLookAndFeel (nullptr); setLookAndFeel (nullptr); }

void LiftoffEditor::timerCallback()
{
    hold.tick (1.0f / 60.0f);
    dial.tick (1.0f / 60.0f);
    dial.repaint(); graph.repaint();
    static int m = 0; if (++m % 6 == 0) autoRow.repaint();
    repaint (getWidth() / 2 - 140, 14, 280, 34); // status pill
    static int n = 0; if (++n % 10 == 0) for (auto& cd : cards) cd->repaint();
}

void LiftoffEditor::paint (juce::Graphics& g)
{
    g.fillAll (c::bg);
    auto r = getLocalBounds().toFloat().reduced (6);
    g.setColour (c::panel); g.fillRoundedRectangle (r, 18);
    g.setColour (c::edge); g.drawRoundedRectangle (r, 18, 1);

    auto lp = textPath ("liftoff", logo (34.0f));
    auto lb = lp.getBounds();
    lp.applyTransform (juce::AffineTransform::translation (28 - lb.getX(), 33 - lb.getCentreY()));
    g.setColour (c::blue); g.fillPath (lp);
    g.setFont (mono (9.5f)); g.setColour (c::dim);
    g.drawText ("buildups that hit", 30, 50, 200, 14, juce::Justification::left);

    // status pill
    auto pill = Rectangle<float> (getWidth() / 2.0f - 120, 16, 240, 30);
    const bool hot = dial.blasting() || proc.uiBuild.load() > 0.95f;
    g.setColour (hot ? c::orange : c::card); g.fillRoundedRectangle (pill, 15);
    g.setColour (hot ? c::orange : c::edge); g.drawRoundedRectangle (pill, 15, 1);
    g.setFont (mono (12.5f)); g.setColour (hot ? juce::Colours::white : c::text);
    g.drawText ("status: " + dial.status(), pill, juce::Justification::centred);

    g.setFont (mono (10.0f)); g.setColour (c::dim);
    g.drawText ("delay time", getWidth() - 236, 16, 80, 30, juce::Justification::centredRight);
}

void LiftoffEditor::resized()
{
    delayTime.setBounds (getWidth() - 150, 16, 120, 30);
    dial.setBounds (24, 62, 270, 330);
    hold.setBounds (44, 396, 230, 32);
    const int x0 = 310, w = getWidth() - x0 - 24;
    const int cw = (w - 5 * 8) / 6;
    for (size_t i = 0; i < cards.size(); ++i) cards[i]->setBounds (x0 + (int) i * (cw + 8), 66, cw, 170);
    graph.setBounds (x0, 248, w, 180);
    autoRow.setBounds (24, 444, getWidth() - 48, 44);
}

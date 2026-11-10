#include "PluginEditor.h"
#include "BinaryData.h"

using namespace utui;

Fonts::Fonts()
{
    black = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBlack_otf, BinaryData::InterBlack_otfSize);
    bold = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBold_otf, BinaryData::InterBold_otfSize);
    medium = juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_otf, BinaryData::InterMedium_otfSize);
    mono = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
}

static juce::String noteName (int m) { return ut::keyNames[(m % 12 + 12) % 12] + juce::String (m / 12 - 1); }
static const juce::String DOT = juce::String::fromUTF8 ("\xc2\xb7");

// ------------------------------------------------------------------ look and feel
LnF::LnF (const Fonts& fonts) : f (fonts)
{
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe9ffc8));
    setColour (juce::TooltipWindow::backgroundColourId, col::module);
    setColour (juce::TooltipWindow::textColourId, col::ink);
    setColour (juce::TooltipWindow::outlineColourId, col::sig);
    setColour (juce::PopupMenu::backgroundColourId, col::module);
    setColour (juce::PopupMenu::textColourId, col::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::moduleHi);
    setColour (juce::PopupMenu::highlightedTextColourId, col::sig);
}

void LnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float r = juce::jmin (w, h) * 0.5f - 7.f;
    const float ang = a0 + pos * (a1 - a0);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * i / 10.f, big = (i == 0 || i == 5 || i == 10) ? 1.f : 0.f;
        const float r0 = r + 3.f, r1 = r + 5.5f + big * 1.5f;
        g.setColour (big > 0 ? col::dim : col::faint);
        g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0, cx + std::sin (a) * r1, cy - std::cos (a) * r1, 1.1f);
    }
    juce::Path track; track.addCentredArc (cx, cy, r, r, 0, a0, a1, true);
    g.setColour (col::edgeDark); g.strokePath (track, juce::PathStrokeType (3.f));
    const bool bip = s.getMinimum() < 0;
    juce::Path val; val.addCentredArc (cx, cy, r, r, 0, bip ? (a0 + a1) * 0.5f : a0, ang, true);
    g.setColour (s.isMouseOverOrDragging() ? col::sig : col::sig.withMultipliedBrightness (0.85f));
    g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    const float cr = r - 5.f;
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillEllipse (cx - cr, cy - cr + 2.f, cr * 2, cr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5058), cx, cy - cr, juce::Colour (0xff202429), cx, cy + cr, false));
    g.fillEllipse (cx - cr, cy - cr, cr * 2, cr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff353a41), cx - cr * 0.5f, cy - cr * 0.6f, juce::Colour (0xff1b1e22), cx + cr * 0.4f, cy + cr * 0.7f, true));
    g.fillEllipse (cx - cr + 3, cy - cr + 3, cr * 2 - 6, cr * 2 - 6);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawEllipse (cx - cr + 0.5f, cy - cr + 0.5f, cr * 2 - 1, cr * 2 - 1, 1.f);
    g.setColour (col::edgeDark); g.drawEllipse (cx - cr, cy - cr, cr * 2, cr * 2, 1.f);
    const float p0 = cr * 0.28f, p1 = cr * 0.86f;
    g.setColour (col::ink);
    g.drawLine (cx + std::sin (ang) * p0, cy - std::cos (ang) * p0, cx + std::sin (ang) * p1, cy - std::cos (ang) * p1, 2.2f);
}

void LnF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& c)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (col::screen); g.fillRoundedRectangle (r, 4);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawHorizontalLine (h - 1, 3.f, w - 3.f);
    if (c.isMouseOver (true)) { g.setColour (col::sig.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1.f); }
    juce::Path tri; const float cx = w - 12.f, cy = h * 0.5f;
    tri.addTriangle (cx - 3.5f, cy - 1.5f, cx + 3.5f, cy - 1.5f, cx, cy + 2.5f);
    g.setColour (col::label); g.fillPath (tri);
}
void LnF::positionComboBoxText (juce::ComboBox& c, juce::Label& l)
{
    l.setBounds (8, 0, c.getWidth() - 24, c.getHeight()); l.setFont (f.n (12.5f)); l.setJustificationType (juce::Justification::centredLeft);
}
void LnF::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::module); g.setColour (col::edgeDark); g.drawRect (0, 0, w, h);
}
void LnF::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool hl, bool ticked, bool, const juce::String& text,
                             const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (sep) { g.setColour (col::edge); g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY())); return; }
    if (hl && active) { g.setColour (col::moduleHi); g.fillRect (area); }
    g.setColour (ticked ? col::sig : (active ? col::ink : col::dim)); g.setFont (f.m (14));
    g.drawText (text, area.withTrimmedLeft (22), juce::Justification::centredLeft);
    if (ticked) { g.setColour (col::sig); g.fillEllipse ((float) area.getX() + 9, (float) area.getCentreY() - 2.5f, 5, 5); }
}

// ------------------------------------------------------------------ knob / button
Knob::Knob (const Fonts& fonts, juce::AudioProcessorValueTreeState& st, const char* id, juce::String lab) : label (lab), f (fonts)
{
    addAndMakeVisible (s);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity (180);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, s);
    s.onValueChange = [this] { repaint(); };
    s.onDragStart = [this] { repaint(); }; s.onDragEnd = [this] { repaint(); };
    auto* prm = st.getParameter (id);
    s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
}
void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::label); g.setFont (f.caps (9.5f));
    g.drawText (label.toUpperCase(), 0, 0, getWidth(), 14, juce::Justification::centred);
    const bool hot = s.isMouseOverOrDragging();
    g.setColour (hot ? col::sig : col::ink.withAlpha (0.85f)); g.setFont (f.n (11));
    g.drawText (s.getTextFromValue (s.getValue()), 0, getHeight() - 15, getWidth(), 14, juce::Justification::centred);
}

void Btn::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool on = getToggleState(), en = isEnabled();
    const auto t = getButtonText();
    if (kind == Primary)
    {
        g.setGradientFill (juce::ColourGradient (col::sig.brighter (over ? 0.12f : 0.04f), 0, r.getY(), col::sig.darker (0.18f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5);
        g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 4, r.getRight() - 4);
        g.setColour (juce::Colour (0xff4b5a1a)); g.drawRoundedRectangle (r, 5, 1.f);
        if (down) { g.setColour (juce::Colours::black.withAlpha (0.12f)); g.fillRoundedRectangle (r, 5); }
        const float s = 16, dx = r.getX() + 20, dy = r.getCentreY() - s / 2;
        g.setColour (juce::Colour (0xff141a06)); g.drawRoundedRectangle (dx, dy, s, s, 3.5f, 1.6f);
        for (auto pt : { juce::Point<float> (4.5f, 4.5f), juce::Point<float> (8, 8), juce::Point<float> (11.5f, 11.5f) })
            g.fillEllipse (dx + pt.x - 1.5f, dy + pt.y - 1.5f, 3, 3);
        auto ft = f.b (15); ft.setExtraKerningFactor (0.08f); g.setFont (ft);
        g.drawText (t, r.withTrimmedLeft (30), juce::Justification::centred);
        return;
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (over && en ? 0xff2d3237 : 0xff272b30), 0, r.getY(), juce::Colour (0xff1d2024), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 4);
    if (down) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r, 4); }
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (r, 4, 1.f);
    g.setColour (! en ? col::faint : (on ? col::sig : col::ink.withAlpha (0.9f)));
    if (t.isEmpty())
    {
        const float cx = r.getCentreX(), cy = r.getCentreY();
        if (on) g.fillRect (cx - 5.f, cy - 5.f, 10.f, 10.f);
        else { juce::Path p; p.addTriangle (cx - 4.5f, cy - 6.5f, cx - 4.5f, cy + 6.5f, cx + 7, cy); g.fillPath (p); }
        return;
    }
    auto ft = f.s (11); ft.setExtraKerningFactor (0.1f); g.setFont (ft);
    if (kind == Toggle)
    {
        const float lx = r.getX() + 11, ly = r.getCentreY();
        if (on) { g.setColour (col::sig.withAlpha (0.25f)); g.fillEllipse (lx - 5, ly - 5, 10, 10); }
        g.setColour (on ? col::sig : col::faint); g.fillEllipse (lx - 3, ly - 3, 6, 6);
        g.setColour (on ? col::ink : col::label);
        g.drawText (t, r.withTrimmedLeft (14), juce::Justification::centred);
    }
    else g.drawText (t, r, juce::Justification::centred);
}

// ------------------------------------------------------------------ style list
static std::vector<int> styleGlyph (int style)
{
    std::vector<int> on (16, 0);
    if (ut::usesBeatCells (style)) { const auto& c = ut::beatCells (style)[0]; for (int b = 0; b < 4; ++b) for (auto& n : c) on[(size_t) (b * 4 + n.s)] = 1; }
    else for (auto& n : ut::barTemplates (style)[0]) on[(size_t) n.s] = 1;
    return on;
}
void StyleList::paint (juce::Graphics& g)
{
    const int sel = (int) p.p ("style"), rh = rowH();
    for (int i = 0; i < ut::NumStyles; ++i)
    {
        auto r = juce::Rectangle<float> (0, (float) (i * rh), (float) getWidth(), (float) rh).reduced (0, 1.5f);
        const bool on = i == sel, hv = i == hover;
        if (on || hv) { g.setColour (on ? col::moduleHi : col::module.brighter (0.04f)); g.fillRoundedRectangle (r, 4); }
        if (on) { g.setColour (col::sig); g.fillRoundedRectangle (r.getX(), r.getY() + 7, 3, r.getHeight() - 14, 1.5f); }
        g.setColour (on ? col::ink : (hv ? col::ink.withAlpha (0.85f) : col::label)); g.setFont (f.s (12.5f));
        g.drawText (ut::styleName (i), r.withTrimmedLeft (14).withTrimmedBottom (r.getHeight() * 0.45f), juce::Justification::bottomLeft);
        const auto gl = styleGlyph (i);
        const float gx = r.getX() + 14, gy = r.getY() + r.getHeight() * 0.62f, cw = 6.5f;
        for (int s = 0; s < 16; ++s)
        {
            const float x = gx + s * cw + (s / 4) * 3.f;
            g.setColour (gl[(size_t) s] ? (on ? col::sig : col::sigDim) : (s % 4 == 0 ? col::faint : col::edge));
            g.fillRect (x, gl[(size_t) s] ? gy : gy + 4.f, 4.5f, gl[(size_t) s] ? 6.f : 2.f);
        }
        g.setColour (on ? col::sigDim : col::faint); g.setFont (f.n (10));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), r.withTrimmedRight (10), juce::Justification::centredRight);
    }
}
void StyleList::mouseDown (const juce::MouseEvent& e)
{
    const int i = rowAt (e.y); if (i < 0) return;
    auto* prm = p.apvts.getParameter ("style");
    prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i)); prm->endChangeGesture();
    repaint();
}

// ------------------------------------------------------------------ preset bar
void PresetBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::screen); g.fillRoundedRectangle (r, 4);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawHorizontalLine ((int) r.getBottom() - 1, 4.f, r.getRight() - 4);
    auto arrow = [&] (float cx, bool left) { juce::Path pa; const float cy = r.getCentreY(); if (left) pa.addTriangle (cx + 3, cy - 5, cx + 3, cy + 5, cx - 4, cy); else pa.addTriangle (cx - 3, cy - 5, cx - 3, cy + 5, cx + 4, cy); g.setColour (col::label); g.fillPath (pa); };
    arrow (18, true); arrow (r.getRight() - 18, false);
    g.setColour (col::edge); g.drawVerticalLine (34, 6, r.getBottom() - 6); g.drawVerticalLine ((int) r.getRight() - 35, 6, r.getBottom() - 6);
    const auto& L = UndertowProcessor::presets();
    const bool none = p.currentPreset < 0;
    const int i = none ? 0 : juce::jlimit (0, (int) L.size() - 1, p.currentPreset);
    g.setColour (col::dim); g.setFont (f.n (10.5f));
    g.drawText (none ? juce::String ("--") : juce::String (i + 1).paddedLeft ('0', 2), 44, 0, 30, getHeight(), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffe9ffc8)); g.setFont (f.s (14));
    const juce::String yours = p.activeUserPreset.isNotEmpty() ? p.activeUserPreset : juce::String ("Your bassline");
    g.drawText (none ? yours : juce::String (L[(size_t) i].name) + (p.presetModified || p.isEdited() ? " *" : ""), 70, 0, getWidth() - 140, getHeight(), juce::Justification::centredLeft);
    if (! none)
    {
        g.setColour (col::dim); g.setFont (f.caps (11.f));
        g.drawText ("FACTORY", 0, 0, getWidth() - 46, getHeight(), juce::Justification::centredRight);
    }
}
void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    if (e.x < 35) { p.applyPreset (p.currentPreset < 0 ? (int) UndertowProcessor::presets().size() - 1 : p.currentPreset - 1); repaint(); return; }
    if (e.x > getWidth() - 35) { p.applyPreset (p.currentPreset < 0 ? 0 : p.currentPreset + 1); repaint(); return; }
    p.refreshUserPresets();
    juce::PopupMenu m; const auto& L = UndertowProcessor::presets();
    m.addSectionHeader ("FACTORY");
    for (int i = 0; i < (int) L.size(); ++i) m.addItem (i + 1, L[(size_t) i].name, true, i == p.currentPreset);
    m.addSeparator();
    m.addItem (1000, "Save preset...");
    if (! p.userPresets.empty())
    {
        m.addSeparator();
        m.addSectionHeader ("YOURS");
        for (int i = 0; i < (int) p.userPresets.size(); ++i)
        {
            juce::PopupMenu sub;
            sub.addItem (2000 + i, "Load");
            sub.addItem (4000 + i, "Rename");
            sub.addItem (3000 + i, "Delete");
            m.addSubMenu (p.userPresets[(size_t) i].name, sub);
        }
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                     [this] (int r) {
                         if (r <= 0) return;
                         if (r < 1000) { p.activeUserPreset.clear(); p.applyPreset (r - 1); repaint(); return; }
                         if (r == 1000)
                         {
                             auto* aw = new juce::AlertWindow ("Save preset", "Name this bassline", juce::AlertWindow::NoIcon);
                             aw->addTextEditor ("name", p.activeUserPreset.isNotEmpty() ? p.activeUserPreset : "My bassline");
                             aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
                             aw->addButton ("Cancel", 0);
                             aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int res) {
                                 if (res == 1) p.saveUserPreset (aw->getTextEditorContents ("name"));
                                 repaint();
                             }), true);
                             return;
                         }
                         if (r >= 4000) { const int i = r - 4000;
                             auto* aw = new juce::AlertWindow ("Rename preset", "New name", juce::AlertWindow::NoIcon);
                             aw->addTextEditor ("name", p.userPresets[(size_t) i].name);
                             aw->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
                             aw->addButton ("Cancel", 0);
                             aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw, i] (int res) {
                                 if (res == 1) p.renameUserPreset (i, aw->getTextEditorContents ("name"));
                                 repaint();
                             }), true);
                             return; }
                         if (r >= 3000) { p.deleteUserPreset (r - 3000); repaint(); return; }
                         if (r >= 2000) { p.applyUserPreset (r - 2000); repaint(); }
                     });
}

// ------------------------------------------------------------------ piano roll (editable)
juce::Rectangle<float> Roll::gridArea() const { return { 52.f, 76.f, getWidth() - 64.f, getHeight() - 76.f - 66.f - 40.f }; }
juce::Rectangle<float> Roll::velArea() const { auto gA = gridArea(); return { gA.getX(), gA.getBottom() + 10.f, gA.getWidth(), 56.f }; }
float Roll::xOfStep (double step) const { auto gA = gridArea(); return gA.getX() + (float) ((step - scroll) * gA.getWidth() / viewSteps()); }
int Roll::stepAt (float x) const { auto gA = gridArea(); return juce::jlimit (0, p.bars() * 16 - 1, scroll + (int) std::floor ((x - gA.getX()) / (gA.getWidth() / viewSteps()))); }
void Roll::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float d = w.deltaY != 0.f ? w.deltaY : w.deltaX;
    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        zoom = d > 0 ? std::min (4.f, zoom * 2.f) : std::max (1.f, zoom * 0.5f);
    else
        scroll -= (int) std::copysign (std::max (4, viewSteps() / 8), d);
    const int vis = viewSteps();
    scroll = juce::jlimit (0, std::max (0, p.bars() * 16 - vis), scroll);
    repaint();
}
int Roll::degAt (float y) const { auto gA = gridArea(); const float rh = gA.getHeight() / (hi - lo + 1); return juce::jlimit (lo, hi, hi - (int) std::floor ((y - gA.getY()) / rh)); }
juce::Rectangle<float> Roll::noteRect (const ut::Note& n) const
{
    auto gA = gridArea(); const float rh = gA.getHeight() / (hi - lo + 1), sw = p.swing(), cw = gA.getWidth() / viewSteps();
    const float off = (n.step % 2) ? (sw - 0.5f) * 2.f * cw : 0.f;
    const int st = (int) p.p ("style");
    const float pocket = ((n.step % 4) == 2 && (st == ut::Offbeat || st == ut::DeepTech || st == ut::Rolling || st == ut::Minimal)) ? -0.04f * cw : 0.f;
    const float x0 = xOfStep (n.step) + off + pocket + 1.f, x1 = xOfStep (n.step + n.len) - 1.f;
    return { x0, gA.getY() + (hi - n.deg) * rh + 1.5f, std::max (5.f, x1 - x0), rh - 3.f };
}
int Roll::noteAt (juce::Point<float> pt) const
{
    for (int i = (int) pat.size() - 1; i >= 0; --i) if (noteRect (pat[(size_t) i]).expanded (1, 1).contains (pt)) return i;
    return -1;
}
void Roll::computeRange()
{
    int a = 0, b = 7;
    for (auto& n : pat) { a = std::min (a, n.deg); b = std::max (b, n.deg); }
    lo = std::max (-10, a - 2); hi = std::min (21, b + 2);
    while (hi - lo < 13) { if (hi < 21) ++hi; if (hi - lo < 13 && lo > -10) --lo; }
}
void Roll::refresh()
{
    if (drag == None && p.patternVersion.load() != version) { pat = p.getPattern(); version = p.patternVersion.load(); computeRange(); for (auto& x : flash) x = 0; }
    const int idx = p.uiNoteIdx.exchange (-1);
    if (idx >= 0 && idx < 2048) flash[idx] = 1.f;
    for (auto& x : flash) x *= 0.84f;
}

void Roll::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int bars = p.bars(), steps = bars * 16, root = p.rootMidi(), sc = p.scale(), key = (int) p.p ("key");
    auto gA = gridArea(); const float cw = gA.getWidth() / viewSteps(), rh = gA.getHeight() / (hi - lo + 1);
    auto scr = b.withTrimmedTop (28);
    g.setColour (col::screen); g.fillRoundedRectangle (scr, 5);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (scr.reduced (0.5f), 5, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) scr.getBottom() - 1, scr.getX() + 5, scr.getRight() - 5);
    g.setColour (col::label); g.setFont (f.caps (11.f)); g.drawText ("PATTERN", 0, 2, 90, 18, juce::Justification::left);
    g.setColour (col::dim); g.setFont (f.n (11.f));
    g.drawText (zoom > 1.01f ? juce::String (juce::roundToInt (zoom)) + "x   ctrl-wheel zoom, wheel scrolls" : "ctrl-wheel zoom, wheel scrolls",
                (int) b.getRight() - 280, 2, 270, 18, juce::Justification::right);
    g.setColour (col::dim); g.setFont (f.n (11));
    int ghosts = 0; for (auto& n : pat) ghosts += n.ghost ? 1 : 0;
    g.drawText (juce::String ((int) pat.size()) + " notes  " + DOT + "  " + juce::String (ghosts) + " ghost  " + DOT + "  " + ut::keyNames[key] + " " + ut::scaleName (sc)
                    + "  " + DOT + "  " + juce::String (ut::styleName ((int) p.p ("style"))).toLowerCase(), 86, 2, 250, 18, juce::Justification::left);
    if (p.isEdited())
    {
        auto e = juce::Rectangle<float> (b.getRight() - 64, 2, 64, 18);
        g.setColour (col::warn.withAlpha (0.12f)); g.fillRoundedRectangle (e, 3); g.setColour (col::warn); g.drawRoundedRectangle (e.reduced (0.5f), 3, 1);
        g.setFont (f.caps (9)); g.drawText ("EDITED", e, juce::Justification::centred);
    }
    // chord row
    int roots[4]; p.getBarRoots (roots);
    for (int bar = 0; bar < bars; ++bar)
    {
        const int sh = roots[std::min (3, bar % std::min (bars, 4))], prevSh = bar > 0 ? roots[std::min (3, (bar - 1) % std::min (bars, 4))] : 999;
        const bool compact = bars >= 8;
        const bool spelled = sc == ut::MinorPent || sc == ut::Major || sc == ut::HarmMinor;
        const int rs = spelled ? ut::chordSemis (sh) : ut::degToSemi (sh, sc);
        const int third = (spelled ? ut::chordSemis (sh + 2) : ut::degToSemi (sh + 2, sc)) - rs;
        const int fifth = (spelled ? ut::chordSemis (sh + 4) : ut::degToSemi (sh + 4, sc)) - rs;
        const juce::String q = fifth == 6 ? "dim" : (third == 3 ? "m" : "");
        const float x0 = xOfStep (bar * 16), x1 = xOfStep ((bar + 1) * 16);
        g.setColour (compact && sh == prevSh ? col::faint : col::ink); g.setFont (f.s (compact ? 11.f : 12.5f));
        static const char* flatNames[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        g.drawText (juce::String (flatNames[((key + rs) % 12 + 12) % 12]) + q, (int) x0 + 6, 38, 80, 16, juce::Justification::left);
        static const char* rom[] = { "i", "ii", "iii", "iv", "v", "vi", "vii" };
        static const char* byPc[] = { "i", "bii", "ii", "bIII", "III", "iv", "bV", "v", "bVI", "VI", "bVII", "vii" };
        juce::String rn = spelled ? byPc[(rs % 12 + 12) % 12] : juce::String (rom[((sh % 7) + 7) % 7]);
        if (! spelled && third == 4) rn = rn.toUpperCase();
        if (! compact) { g.setColour (col::dim); g.setFont (f.n (10)); g.drawText (rn, (int) x1 - 46, 38, 40, 16, juce::Justification::right); }
        if (bar > 0) { g.setColour (bar % 4 == 0 ? col::sigDim : col::screenBar); g.fillRect (x0, 36.f, 1.f, 20.f); }
    }
    // kick row follows the style: 4/4, 2-step or broken
    const bool kickOn = p.p ("kick") > 0.5f;
    const int kickStyle = (int) p.p ("style");
    for (int s = 0; s < steps; ++s) if (ut::isKickStep (kickStyle, s))
    {
        const float x = xOfStep (s) + cw * 0.35f;
        g.setColour (kickOn ? col::sigDim : col::faint); g.fillEllipse (x - 2.5f, gA.getY() - 11, 5, 5);
    }
    g.setColour (col::dim); g.setFont (f.caps (11.f)); g.drawText ("KICK", 4, (int) gA.getY() - 16, (int) gA.getX() - 10, 14, juce::Justification::right);
    for (int d = lo; d <= hi; ++d)
    {
        const float y = gA.getY() + (hi - d) * rh;
        const bool isRoot = ((d % 7) + 7) % 7 == 0;
        g.setColour (isRoot ? juce::Colour (0xff141910) : ((((d % 2) + 2) % 2) ? juce::Colour (0xff0e1012) : col::screen));
        g.fillRect (gA.getX(), y, gA.getWidth(), rh);
        if (rh >= 11 || isRoot)
        {
            g.setColour (isRoot ? col::sigDim : col::dim); g.setFont (f.n (isRoot ? 12.f : 11.f));
            g.drawText (noteName (root + ut::degToSemi (d, sc)), 4, (int) y, (int) gA.getX() - 10, (int) rh, juce::Justification::right);
        }
    }
    for (int s = 0; s <= steps; ++s)
    {
        const float x = xOfStep (s);
        g.setColour (s % 16 == 0 ? col::screenBar : (s % 4 == 0 ? col::screenBeat : col::screenLine));
        g.fillRect (x - (s % 16 == 0 ? 0.5f : 0.f), gA.getY(), s % 16 == 0 ? 1.5f : 1.f, gA.getHeight());
    }
    for (size_t i = 1; i < pat.size(); ++i)
        if (pat[i].slide)
        {
            const auto a = noteRect (pat[i - 1]), c = noteRect (pat[i]);
            juce::Path pth; pth.startNewSubPath (a.getRight() - 4, a.getCentreY()); pth.cubicTo (a.getRight() + 6, a.getCentreY(), c.getX() - 6, c.getCentreY(), c.getX() + 4, c.getCentreY());
            g.setColour (col::ink.withAlpha (0.55f)); g.strokePath (pth, juce::PathStrokeType (1.5f));
        }
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& n = pat[i]; auto r = noteRect (n);
        const float fl = i < 2048 ? flash[i] : 0.f, v = n.vel / 127.f;
        const bool hv = (int) i == hoverNote || (int) i == dragIdx;
        if (n.ghost)
        {
            g.setColour (col::sig.withAlpha (0.12f + 0.4f * fl)); g.fillRoundedRectangle (r, 2.5f);
            g.setColour (col::sig.withAlpha (hv ? 0.9f : 0.55f)); g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, 1.f);
            continue;
        }
        const auto body = col::sigDim.interpolatedWith (col::sig, 0.35f + 0.65f * v).interpolatedWith (juce::Colour (0xfff2f8dc), fl * 0.8f);
        g.setColour (body); g.fillRoundedRectangle (r, 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.22f)); g.fillRect (r.getX() + 2, r.getY() + 1, r.getWidth() - 4, 1.f);
        g.setColour (hv ? col::ink : juce::Colour (0xff3d4a14)); g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, hv ? 1.4f : 1.f);
        if (n.slide) { g.setColour (juce::Colour (0xff1b220a)); g.fillEllipse (r.getX() + 3, r.getCentreY() - 2, 4, 4); }
    }
    auto vA = velArea();
    g.setColour (juce::Colour (0xff0d0f11)); g.fillRect (vA);
    g.setColour (col::screenLine); g.drawHorizontalLine ((int) vA.getY(), vA.getX(), vA.getRight());
    g.setColour (col::dim); g.setFont (f.caps (11.f)); g.drawText ("VEL", 4, (int) vA.getY() + 2, (int) vA.getX() - 10, 14, juce::Justification::right);
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& n = pat[i]; const auto r = noteRect (n);
        const float x = r.getX() + 2.5f, h = (vA.getHeight() - 10) * n.vel / 127.f, fl = i < 2048 ? flash[i] : 0.f;
        g.setColour ((n.ghost ? col::sigDim : col::sig).interpolatedWith (col::ink, fl * 0.6f));
        g.fillRect (x - 1.f, vA.getBottom() - 4 - h, 2.f, h);
        g.fillRect (x - 3.f, vA.getBottom() - 4 - h, 6.f, 2.f);
    }
    g.setFont (f.n (10));
    for (int s = 0; s < steps; s += (bars >= 8 ? 16 : 4))
    {
        g.setColour (s % 16 == 0 ? col::label : col::faint);
        g.drawText (juce::String (s / 16 + 1) + (s % 16 ? "." + juce::String (s % 16 / 4 + 1) : ""), (int) xOfStep (s) + 3, (int) vA.getBottom() + 4, 40, 14, juce::Justification::left);
    }
    g.setColour (col::dim); g.setFont (f.n (9.5f));
    g.drawText ("CLICK add  " + DOT + "  DRAG move  " + DOT + "  EDGE resize  " + DOT + "  RIGHT-CLICK delete  " + DOT + "  ALT-CLICK slide  " + DOT + "  VEL lane: drag",
                0, getHeight() - 18, getWidth(), 14, juce::Justification::centred);
    const float beat = p.uiBeat.load();
    if (beat >= 0)
    {
        const float x = xOfStep (beat * 4.0);
        g.setColour (col::sig.withAlpha (0.10f)); g.fillRect (x - 6, gA.getY(), 12.f, vA.getBottom() - gA.getY());
        g.setColour (col::sig); g.fillRect (x - 0.75f, gA.getY() - 14, 1.5f, vA.getBottom() - gA.getY() + 14);
    }
}

void Roll::mouseMove (const juce::MouseEvent& e)
{
    const int h = noteAt (e.position);
    if (h != hoverNote) { hoverNote = h; repaint(); }
    if (h >= 0 && e.position.x > noteRect (pat[(size_t) h]).getRight() - 6) setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    else if (h >= 0) setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    else if (gridArea().contains (e.position)) setMouseCursor (juce::MouseCursor::CrosshairCursor);
    else if (velArea().contains (e.position)) setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    else setMouseCursor (juce::MouseCursor::NormalCursor);
}

void Roll::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = e.position; const int total = p.bars() * 16;
    if (velArea().contains (pt)) { p.beginEdit(); drag = Velocity; mouseDrag (e); return; }
    if (! gridArea().contains (pt)) return;
    const int idx = noteAt (pt);
    if (e.mods.isPopupMenu())
    {
        if (idx >= 0) { p.beginEdit(); pat.erase (pat.begin() + idx); p.applyEdit (pat); hoverNote = -1; repaint(); }
        return;
    }
    if (idx >= 0 && e.mods.isAltDown())
    {
        p.beginEdit(); auto& n = pat[(size_t) idx]; n.slide = ! n.slide;
        if (n.slide && idx > 0) pat[(size_t) idx - 1].len = std::max (1, n.step - pat[(size_t) idx - 1].step);
        p.applyEdit (pat); repaint(); return;
    }
    if (idx >= 0)
    {
        p.beginEdit();
        dragIdx = idx; orig = pat[(size_t) idx];
        drag = pt.x > noteRect (orig).getRight() - 6 ? Resize : Move;
        grabStepOff = stepAt (pt.x) - orig.step;
    }
    else
    {
        ut::Note n; n.step = stepAt (pt.x); n.deg = degAt (pt.y); n.vel = 104;
        for (auto& o : pat) if (o.step == n.step) return; // one note per step: it's a bassline
        int next = total; for (auto& o : pat) if (o.step > n.step) next = std::min (next, o.step);
        n.len = std::max (1, std::min (2, next - n.step));
        p.beginEdit();
        pat.push_back (n); dragIdx = (int) pat.size() - 1; orig = n; drag = Resize; grabStepOff = 0;
        p.applyEdit (pat);
    }
    repaint();
}

void Roll::mouseDrag (const juce::MouseEvent& e)
{
    const auto pt = e.position; const int total = p.bars() * 16;
    if (drag == Velocity)
    {
        auto vA = velArea(); const float v = juce::jlimit (0.f, 1.f, (vA.getBottom() - 4 - pt.y) / (vA.getHeight() - 10));
        int best = -1; float bd = 1e9f;
        for (size_t i = 0; i < pat.size(); ++i) { const float dx = std::abs (noteRect (pat[i]).getX() + 2.5f - pt.x); if (dx < bd) { bd = dx; best = (int) i; } }
        if (best >= 0 && bd < 14) { pat[(size_t) best].vel = juce::jlimit (30, 127, (int) std::round (v * 127)); p.applyEdit (pat); repaint(); }
        return;
    }
    if (dragIdx < 0 || dragIdx >= (int) pat.size()) return;
    auto& n = pat[(size_t) dragIdx];
    if (drag == Move)
    {
        const int ns = juce::jlimit (0, total - n.len, stepAt (pt.x) - grabStepOff), nd = degAt (pt.y);
        bool clash = false; for (size_t i = 0; i < pat.size(); ++i) if ((int) i != dragIdx && pat[i].step == ns) clash = true;
        if (! clash && (ns != n.step || nd != n.deg)) { n.step = ns; n.deg = nd; p.applyEdit (pat); }
    }
    else if (drag == Resize)
    {
        const int nl = juce::jlimit (1, total - n.step, stepAt (pt.x) - n.step + 1);
        if (nl != n.len) { n.len = nl; p.applyEdit (pat); }
    }
    repaint();
}

void Roll::mouseUp (const juce::MouseEvent&)
{
    if (drag != None)
    {
        std::sort (pat.begin(), pat.end(), [] (const ut::Note& a, const ut::Note& b) { return a.step < b.step; });
        for (size_t i = 0; i + 1 < pat.size(); ++i) pat[i].len = std::max (1, std::min (pat[i].len, pat[i + 1].step - pat[i].step));
        p.applyEdit (pat); version = p.patternVersion.load();
    }
    drag = None; dragIdx = -1; repaint();
}

void Roll::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int idx = noteAt (e.position);
    if (idx >= 0) { p.beginEdit(); pat.erase (pat.begin() + idx); p.applyEdit (pat); hoverNote = -1; repaint(); }
}

// ------------------------------------------------------------------ drag tile + scope
void DragTile::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isMouseOver() ? juce::Colour (0xff12160d) : col::screen); g.fillRoundedRectangle (r, 4);
    juce::Path border; border.addRoundedRectangle (r, 4);
    juce::Path dashed; const float d[] = { 4, 3 }; juce::PathStrokeType (1.f).createDashedStroke (dashed, border, d, 2);
    g.setColour (col::sigDim); g.fillPath (dashed);
    auto pat = p.getPattern(); const int steps = p.bars() * 16;
    const float px = r.getX() + 10, pw = 30, py = r.getCentreY() - 8;
    int lo = 99, hi = -99; for (auto& n : pat) { lo = std::min (lo, n.deg); hi = std::max (hi, n.deg); }
    for (auto& n : pat)
    {
        const float x = px + pw * n.step / steps, y = py + (hi > lo ? 14.f * (hi - n.deg) / (hi - lo) : 7.f);
        g.setColour (n.ghost ? col::sigDim : col::sig); g.fillRect (x, y, std::max (1.5f, pw * n.len / steps), 2.f);
    }
    auto ft = f.s (11); ft.setExtraKerningFactor (0.1f);
    g.setColour (col::ink); g.setFont (ft); g.drawText ("DRAG MIDI", (int) px + 38, (int) r.getY() + 7, 100, 14, juce::Justification::left);
    g.setColour (col::dim); g.setFont (f.n (9.5f));
    const int tk = p.printTakes(), tb = p.bars() * tk;
    g.drawText (juce::String (tb) + " bar" + (tb > 1 ? "s" : "") + (tk > 1 ? " " + DOT + " " + juce::String (tk) + " takes" : " " + DOT + " " + juce::String (juce::roundToInt (p.bpm.load())) + " bpm"),
                (int) px + 38, (int) r.getY() + 22, 116, 13, juce::Justification::left);
}
void DragTile::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Undertow"); dir.createDirectory();
    auto f2 = dir.getChildFile (p.suggestedFileName());
    if (p.writeMidi (f2, p.printTakes())) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f2.getFullPathName() }, false, this);
}
void SlotBar::paint (juce::Graphics& g)
{
    g.setColour (col::label); g.setFont (f.caps (9)); g.drawText ("IDEAS", 0, 0, LW - 4, getHeight(), juce::Justification::right);
    const float w = (getWidth() - LW) / (float) UndertowProcessor::NumSlots;
    for (int i = 0; i < UndertowProcessor::NumSlots; ++i)
    {
        auto r = juce::Rectangle<float> (LW + i * w + 2, 1, w - 4, (float) getHeight() - 2);
        const bool used = p.slotUsed (i), cur = p.currentSlot == i;
        g.setColour (cur ? col::sigDeep : (used ? col::moduleHi : col::screen)); g.fillRoundedRectangle (r, 3);
        g.setColour (cur ? col::sig : (i == hover ? col::label : col::edge)); g.drawRoundedRectangle (r.reduced (0.5f), 3, 1);
        g.setColour (cur ? col::sig : (used ? col::ink : col::faint)); g.setFont (f.n (10.5f));
        g.drawText (used ? juce::String (i + 1) : "+", r, juce::Justification::centred);
    }
}
void SlotBar::mouseDown (const juce::MouseEvent& e)
{
    if (e.x < LW) return;
    const int i = slotAt (e.x);
    if (e.mods.isPopupMenu() || (p.slotUsed (i) && p.currentSlot == i && e.mods.isShiftDown()))
    {
        if (! p.slotUsed (i)) return;
        juce::PopupMenu m; m.addItem (1, "Save current idea here"); m.addItem (2, "Clear slot " + juce::String (i + 1));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, i] (int r) { if (r == 1) p.storeSlot (i); if (r == 2) p.clearSlot (i); repaint(); });
        return;
    }
    if (p.slotUsed (i)) p.recallSlot (i); else p.storeSlot (i);
    repaint();
}

void Scope::paint (juce::Graphics& g)
{
    auto sc = getLocalBounds().toFloat();
    g.setColour (col::screen); g.fillRoundedRectangle (sc, 4);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (sc.reduced (0.5f), 4, 1);
    g.setColour (col::screenLine); g.drawHorizontalLine ((int) sc.getCentreY(), sc.getX() + 6, sc.getRight() - 6);
    const int w0 = p.scopeW.load(); int start = (w0 - 400 + 1024) & 1023;
    for (int k = 0; k < 200; ++k) { const int a = (start + k) & 1023, b2 = (a + 1) & 1023; if (p.scope[(size_t) a] <= 0 && p.scope[(size_t) b2] > 0) { start = a; break; } }
    juce::Path wave; const int N = 200; float mx = 0.05f;
    for (int k = 0; k < N; ++k) mx = std::max (mx, std::abs (p.scope[(size_t) ((start + k) & 1023)]));
    for (int k = 0; k < N; ++k)
    {
        const float x = sc.getX() + 6 + (sc.getWidth() - 12) * k / (N - 1), y = sc.getCentreY() - p.scope[(size_t) ((start + k) & 1023)] / mx * (sc.getHeight() * 0.36f);
        k ? wave.lineTo (x, y) : wave.startNewSubPath (x, y);
    }
    g.setColour (col::sig); g.strokePath (wave, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
    g.setColour (col::dim); g.setFont (f.caps (8)); g.drawText ("OUT", (int) sc.getX() + 7, (int) sc.getY() + 4, 40, 12, juce::Justification::left);
}

// ------------------------------------------------------------------ content
Content::Content (UndertowProcessor& p, const Fonts& f) : proc (p), fonts (f), styles (p, f), preset (p, f), roll (p, f), drag (p, f), scope (p, f)
{
    grain = juce::Image (juce::Image::ARGB, 256, 256, true);
    juce::Random rnd (7);
    for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x) { const auto v = (juce::uint8) rnd.nextInt (256); grain.setPixelAt (x, y, juce::Colour (v, v, v).withAlpha ((juce::uint8) 9)); }

    for (auto* c : std::initializer_list<juce::Component*> { &styles, &preset, &roll, &drag, &scope }) addAndMakeVisible (c);
    auto& st = proc.apvts;
    const std::pair<const char*, const char*> ks[] = { { "density", "Density" }, { "movement", "Movement" }, { "variation", "Variation" }, { "gate", "Length" },
                                                         { "swing", "Swing" }, { "glide", "Glide" }, { "ghosts", "Ghosts" }, { "accent", "Accent" },
                                                         { "tone", "Tone" }, { "drive", "Drive" }, { "volume", "Volume" } };
    for (auto& k : ks) { knobs.push_back (std::make_unique<Knob> (fonts, st, k.first, k.second)); addAndMakeVisible (*knobs.back()); }
    auto combo = [&] (juce::ComboBox& c, const char* id) {
        c.addItemList (st.getParameter (id)->getAllValueStrings(), 1); addAndMakeVisible (c);
        combos.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, c));
    };
    combo (key, "key"); combo (scale, "scale"); combo (octave, "octave"); combo (bars, "bars"); combo (chords, "chords"); combo (sound, "sound"); combo (playMode, "playMode");
    kickGap.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (kickGap);
    gapAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, "kickGap", kickGap);
    for (auto* b : { &generate, &mutate, &undo, &redo, &play, &kick, &lockR, &lockP, &exportB, &clear, &evolve, &keyIn }) addAndMakeVisible (*b);
    addAndMakeVisible (slotBar);
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "keyIn", keyIn));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "evolve", evolve));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "kick", kick));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "lockRhythm", lockR));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "lockPitch", lockP));
    generate.setTooltip ("A new bassline. Low Variation keeps the hook looping.");
    mutate.setTooltip ("Same groove, a few notes change.");
    undo.setTooltip ("Back to the previous bassline.");
    redo.setTooltip ("Forward again.");
    clear.setTooltip ("Wipe the piano roll.");
    play.setTooltip ("Preview without the DAW playing. Stops when the DAW starts.");
    kick.setTooltip ("Preview kick while TRY is running. It stays out of the host and out of bounces. Mute silences it.");
    kickGap.setTooltip ("Silence before each kick, in the synth and in exported MIDI. 12 ms keeps a held note off the kick.");
    playMode.setTooltip ("Always: play with the host. Note Held: the pattern plays while a note is held, and that note sets the root. Synth: Undertow plays the MIDI you send it.");
    roll.setTooltip ("Wheel scrolls the roll. Ctrl-wheel zooms (1x, 2x, 4x).");
    lockR.setTooltip ("Generate keeps this rhythm and writes new notes.");
    lockP.setTooltip ("Generate keeps these notes and writes a new rhythm.");
    exportB.setTooltip ("Save a MIDI file, with swing and slides.");
    evolve.setTooltip ("Each time the loop comes around, play a new answer. The hook stays.");
    keyIn.setTooltip ("Off by default. When on, a note-on sets the root inside Undertow. It does not write the Key parameter.");
    drag.setTooltip ("Drag this onto a clip in FL Studio or Ableton.");
    for (auto& k : knobs)
    {
        const auto lab = k->label;
        juce::String tip;
        if (lab == "Density") tip = "How many notes. Low is sparse, high fills the 16ths.";
        else if (lab == "Movement") tip = "How far the line leaves the root. Low stays on the root and octave.";
        else if (lab == "Variation") tip = "0 loops the hook. Mid is a two-bar answer. High rewrites the phrase.";
        else if (lab == "Length") tip = "Short and plucky, or held out.";
        else if (lab == "Swing") tip = "MPC swing on the off-16ths. Changing style sets a fitting value.";
        else if (lab == "Glide") tip = "How long a slide takes. Slides are the notes with a dot.";
        else if (lab == "Ghosts") tip = "Quiet pickup notes into the main hits.";
        else if (lab == "Accent") tip = "How much louder the offbeat hits than the rest.";
        else if (lab == "Tone") tip = "Filter brightness. Left is darker.";
        else if (lab == "Drive") tip = "Warm saturation. The sub stays clean.";
        else if (lab == "Volume") tip = "Output level.";
        if (tip.isNotEmpty()) k->s.setTooltip (tip);
    }
    generate.onClick = [this] { proc.generateNew(); };
    mutate.onClick = [this] { proc.mutate(); };
    undo.onClick = [this] { proc.undo(); };
    redo.onClick = [this] { proc.redo(); };
    clear.onClick = [this] { proc.beginEdit(); proc.applyEdit ({}); };
    play.onClick = [this] { proc.internalPlay = ! proc.internalPlay.load(); };
    exportB.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Export MIDI", juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.suggestedFileName()), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) { auto f2 = fc.getResult(); if (f2 != juce::File()) proc.writeMidi (f2.withFileExtension ("mid"), proc.printTakes()); });
    };
    setSize (W, H);
}

void Content::module (juce::Graphics& g, juce::Rectangle<float> r, const char* title) const
{
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 1.5f), 7);
    g.setGradientFill (juce::ColourGradient (col::module, 0, r.getY(), col::chassis.brighter (0.02f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7);
    g.setColour (juce::Colours::white.withAlpha (0.045f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 6, r.getRight() - 6);
    g.setColour (col::edgeDark); g.drawRoundedRectangle (r.reduced (0.5f), 7, 1.f);
    if (title != nullptr && *title)
    {
        g.setColour (col::label); g.setFont (fonts.caps (10.5f));
        g.drawText (title, (int) r.getX() + 14, (int) r.getY() + 10, 200, 14, juce::Justification::left);
    }
}

void Content::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (col::chassis, 0, 0, col::bg, 0, (float) H, false)); g.fillAll();
    g.setTiledImageFill (grain, 0, 0, 1.f); g.fillRect (getLocalBounds());
    g.setColour (col::edgeDark); g.drawHorizontalLine (64, 0, (float) W);
    g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine (65, 0, (float) W);
    for (int k = 0; k < 3; ++k)
    {
        juce::Path w; const float y0 = 22.f + k * 7.5f, amp = 3.2f - k * 0.7f;
        for (int i = 0; i <= 32; ++i) { const float x = 22 + i * 0.9f, y = y0 + amp * std::sin (i * 0.38f + k * 0.9f); i ? w.lineTo (x, y) : w.startNewSubPath (x, y); }
        g.setColour (k == 0 ? col::sig : col::ink.withAlpha (0.55f - k * 0.15f));
        g.strokePath (w, juce::PathStrokeType (2.4f - k * 0.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    auto wm = fonts.b (24); wm.setExtraKerningFactor (0.06f);
    g.setColour (col::ink); g.setFont (wm); g.drawText ("UNDERTOW", 62, 12, 200, 28, juce::Justification::left);
    g.setColour (col::label); g.setFont (fonts.caps (11.f)); g.drawText ("BASSLINE GENERATOR", 63, 40, 220, 14, juce::Justification::left);
    auto tw = fonts.s (12); tw.setExtraKerningFactor (0.14f);
    g.setColour (col::ink.withAlpha (0.9f)); g.setFont (tw); g.drawText ("TOKEN WISE", W - 182, 18, 160, 16, juce::Justification::right);
    g.setColour (col::dim); g.setFont (fonts.n (10)); g.drawText ("v2.0", W - 182, 36, 160, 12, juce::Justification::right);
    g.setColour (col::label); g.setFont (fonts.caps (9));
    for (auto [x, t] : { std::pair<int, const char*> { 22, "KEY" }, { 96, "OCT" }, { 158, "SCALE" }, { 290, "BARS" }, { 380, "CHORDS" } })
        g.drawText (t, x + 1, 76, 120, 12, juce::Justification::left);
    g.drawText ("TEMPO", 968, 76, 60, 12, juce::Justification::left);
    g.setColour (col::ink); g.setFont (fonts.n (17));
    g.drawText (juce::String (proc.bpm.load(), 1), 968, 90, 80, 24, juce::Justification::left);
    g.setColour (col::dim); g.setFont (fonts.caps (11.f));
    g.drawText (proc.uiBeat.load() >= 0 ? "PLAYING" : "TRY " + juce::String (ut::styleBpm ((int) proc.p ("style"))), 1030, 96, 90, 14, juce::Justification::left);
    module (g, { 14, 128, 228, 552 }, "STYLE  (GROOVE TYPE)");
    module (g, { 252, 128, 662, 552 }, nullptr);
    module (g, { 924, 128, 242, 172 }, "SHAPE");
    module (g, { 924, 306, 242, 172 }, "FEEL");
    module (g, { 924, 484, 242, 196 }, "SOUND");
    module (g, { 14, 690, 1152, 76 }, nullptr);
    g.setColour (col::dim); g.setFont (fonts.m (11.5f));
    g.drawFittedText (juce::String (ut::styleBlurb ((int) proc.p ("style"))) + "   " + juce::String (ut::styleBpm ((int) proc.p ("style"))) + " BPM",
                      28, 648, 200, 26, juce::Justification::centredLeft, 2);
    g.setColour (col::label); g.setFont (fonts.caps (9));
    g.drawText ("LOCK", 600, 722, 40, 12, juce::Justification::left);
    // what each control does, in plain words
    g.setColour (col::dim); g.setFont (fonts.m (11.f));
    g.drawText ("brand-new bassline", 28, 754, 220, 14, juce::Justification::centred);
    g.drawText ("same groove, new notes", 240, 748, 132, 14, juce::Justification::centred);
    g.drawText ("keep these when you generate", 636, 748, 190, 14, juce::Justification::centred);
    g.setColour (col::label); g.setFont (fonts.caps (11.f)); g.drawText ("PLAYBACK", 560, 74, 90, 14, juce::Justification::left);
    g.drawText ("GAP", 856, 74, 50, 14, juce::Justification::left);
}

void Content::resized()
{
    preset.setBounds (390, 14, 400, 36);
    key.setBounds (22, 90, 66, 28); octave.setBounds (96, 90, 54, 28); scale.setBounds (158, 90, 124, 28);
    bars.setBounds (290, 90, 82, 28); chords.setBounds (380, 90, 170, 28);
    play.setBounds (912, 88, 44, 32);
    kick.setBounds (1094, 88, 70, 32);
    evolve.setBounds (560, 88, 88, 30); keyIn.setBounds (652, 88, 88, 30);
    playMode.setBounds (744, 88, 100, 30); kickGap.setBounds (856, 102, 52, 14);
    slotBar.setBounds (578, 139, 250, 20);
    styles.setBounds (22, 152, 212, 492);
    roll.setBounds (266, 136, 634, 536);
    const int kx[] = { 936, 1048 };
    for (int i = 0; i < 4; ++i) knobs[(size_t) i]->setBounds (kx[i % 2], 156 + (i / 2) * 70, 104, 70);
    for (int i = 0; i < 4; ++i) knobs[(size_t) (4 + i)]->setBounds (kx[i % 2], 334 + (i / 2) * 70, 104, 70);
    sound.setBounds (938, 510, 214, 28);
    for (int i = 0; i < 3; ++i) knobs[(size_t) (8 + i)]->setBounds (934 + i * 74, 544, 74, 70);
    scope.setBounds (938, 618, 214, 50);
    generate.setBounds (28, 704, 220, 48);
    mutate.setBounds (258, 712, 96, 32); undo.setBounds (362, 712, 70, 32); redo.setBounds (438, 712, 70, 32); clear.setBounds (514, 712, 74, 32);
    lockR.setBounds (636, 712, 98, 32); lockP.setBounds (740, 712, 86, 32);
    drag.setBounds (900, 706, 160, 44); exportB.setBounds (1068, 712, 86, 32);
}

void Content::tick()
{
    roll.refresh(); roll.repaint();
    play.setToggleState (proc.internalPlay.load() || proc.uiBeat.load() >= 0, juce::dontSendNotification);
    undo.setEnabled (proc.canUndo()); redo.setEnabled (proc.canRedo());
    static int n = 0;
    if (++n % 6 == 0) { styles.repaint(); preset.repaint(); drag.repaint(); slotBar.repaint(); repaint (900, 70, 280, 50); repaint (20, 640, 220, 40); }
    scope.repaint();
}

// ------------------------------------------------------------------ editor (scales the content)
UndertowEditor::UndertowEditor (UndertowProcessor& p) : AudioProcessorEditor (&p), proc (p), tips (this, 450)
{
    setLookAndFeel (&lnf);
    tips.setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) utui::Content::W / utui::Content::H);
        c->setSizeLimits (utui::Content::W, utui::Content::H, utui::Content::W * 3 / 2, utui::Content::H * 3 / 2);
    }
    setSize (utui::Content::W, utui::Content::H);
    startTimerHz (60);
}
UndertowEditor::~UndertowEditor() { proc.internalPlay = false; tips.setLookAndFeel (nullptr); setLookAndFeel (nullptr); } // closing the window stops the preview
void UndertowEditor::resized()
{
    const float s = (float) getWidth() / utui::Content::W;
    content.setBounds (0, 0, utui::Content::W, utui::Content::H);
    content.setTransform (juce::AffineTransform::scale (s));
}
void UndertowEditor::timerCallback() { content.tick(); }

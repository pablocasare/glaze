#include "PluginEditor.h"
#include "BinaryData.h"

using namespace utui;
static constexpr int W = 1160, H = 760;

Fonts::Fonts()
{
    black = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBlack_otf, BinaryData::InterBlack_otfSize);
    bold = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBold_otf, BinaryData::InterBold_otfSize);
    medium = juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_otf, BinaryData::InterMedium_otfSize);
    mono = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
}

// ------------------------------------------------------------------ look and feel
LnF::LnF (const Fonts& fonts) : f (fonts)
{
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::lime);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
}

void LnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const float r = juce::jmin (w, h) * 0.5f - 4, cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    juce::Path track; track.addCentredArc (cx, cy, r, r, 0, a0, a1, true);
    g.setColour (col::line); g.strokePath (track, juce::PathStrokeType (5, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0;
    juce::Path val; val.addCentredArc (cx, cy, r, r, 0, bip ? (a0 + a1) * 0.5f : a0, ang, true);
    g.setColour (col::lime.withAlpha (0.25f)); g.strokePath (val, juce::PathStrokeType (11, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col::lime); g.strokePath (val, juce::PathStrokeType (5, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float br = r - 9;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2d34), cx, cy - br, juce::Colour (0xff15171b), cx, cy + br, false));
    g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
    g.setColour (juce::Colour (0xff33363e)); g.drawEllipse (cx - br, cy - br, br * 2, br * 2, 1.2f);
    const float ix = cx + std::sin (ang) * (br - 5), iy = cy - std::cos (ang) * (br - 5);
    const float ox = cx + std::sin (ang) * (br * 0.35f), oy = cy - std::cos (ang) * (br * 0.35f);
    g.setColour (col::cream); g.drawLine (ox, oy, ix, iy, 3.f);
}

void LnF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& c)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (c.isMouseOver() ? juce::Colour (0xff20232a) : col::panel2); g.fillRoundedRectangle (r, 9);
    g.setColour (col::line); g.drawRoundedRectangle (r, 9, 1);
    juce::Path ch; const float cx = w - 16.f, cy = h * 0.5f;
    ch.startNewSubPath (cx - 4, cy - 2); ch.lineTo (cx, cy + 2); ch.lineTo (cx + 4, cy - 2);
    g.setColour (col::lime); g.strokePath (ch, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
void LnF::positionComboBoxText (juce::ComboBox& c, juce::Label& l)
{
    l.setBounds (10, 0, c.getWidth() - 30, c.getHeight()); l.setFont (f.s (14)); l.setJustificationType (juce::Justification::centredLeft);
}
void LnF::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2); g.setColour (col::line); g.drawRect (0, 0, w, h);
}

// ------------------------------------------------------------------ knob / chip
Knob::Knob (const Fonts& fonts, juce::AudioProcessorValueTreeState& st, const char* id, juce::String lab) : label (lab), f (fonts)
{
    addAndMakeVisible (s);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, s);
    s.onValueChange = [this] { repaint(); };
}
void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::grey); g.setFont (f.s (11.5f));
    g.drawText (label.toUpperCase(), 0, 0, getWidth(), 16, juce::Justification::centred);
    g.setColour (col::cream); g.setFont (f.n (12.5f));
    g.drawText (s.getTextFromValue (s.getValue()), 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred);
}

void Chip::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    const bool on = getToggleState();
    const auto t = getButtonText();
    if (t == "GENERATE")
    {
        g.setColour (col::lime.withAlpha (over ? 0.35f : 0.18f)); g.fillRoundedRectangle (r.expanded (0), 14);
        g.setGradientFill (juce::ColourGradient (col::lime.brighter (0.15f), r.getX(), r.getY(), juce::Colour (0xffa9d23c), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r.reduced (down ? 3.f : 2.f), 12);
        // dice
        auto d = juce::Rectangle<float> (r.getX() + 26, r.getCentreY() - 10, 20, 20);
        g.setColour (juce::Colour (0xff0e1206)); g.drawRoundedRectangle (d, 4, 2.2f);
        for (auto pt : { juce::Point<float> (6, 6), juce::Point<float> (14, 14), juce::Point<float> (10, 10) })
            g.fillEllipse (d.getX() + pt.x - 1.8f, d.getY() + pt.y - 1.8f, 3.6f, 3.6f);
        g.setFont (f.b (21)); g.drawText (t, r.withTrimmedLeft (36), juce::Justification::centred);
        return;
    }
    if (t.isEmpty()) // play button
    {
        g.setColour (on ? col::lime : col::panel2); g.fillEllipse (r);
        g.setColour (on ? col::lime : col::line); g.drawEllipse (r, 1.2f);
        g.setColour (on ? juce::Colours::black : col::lime);
        const float cx = r.getCentreX(), cy = r.getCentreY();
        if (on) g.fillRoundedRectangle (cx - 7, cy - 7, 14, 14, 2);
        else { juce::Path p; p.addTriangle (cx - 5, cy - 9, cx - 5, cy + 9, cx + 10, cy); g.fillPath (p); }
        return;
    }
    if (on && filled) { g.setColour (col::lime); g.fillRoundedRectangle (r, 10); }
    else
    {
        g.setColour (over ? juce::Colour (0xff20232a) : col::panel2); g.fillRoundedRectangle (r, 10);
        g.setColour (on ? col::lime : (over ? col::faint : col::line)); g.drawRoundedRectangle (r, 10, on ? 1.6f : 1.f);
    }
    g.setColour (on && filled ? juce::Colour (0xff0e1206) : (on ? col::lime : col::text));
    if (sub.isNotEmpty())
    {
        g.setFont (f.b (14)); g.drawText (t, r.withTrimmedBottom (r.getHeight() * 0.38f).withTrimmedLeft (12), juce::Justification::bottomLeft);
        g.setColour (on && filled ? juce::Colour (0xff3d4a14) : col::faint); g.setFont (f.n (10.5f));
        g.drawText (sub, r.withTrimmedTop (r.getHeight() * 0.58f).withTrimmedLeft (12), juce::Justification::topLeft);
    }
    else { g.setFont (f.b (13.5f)); g.drawText (t, r, juce::Justification::centred); }
}

// ------------------------------------------------------------------ piano roll
void Roll::refresh()
{
    if (p.patternVersion.load() != version) { pat = p.getPattern(); version = p.patternVersion.load(); for (auto& x : flash) x = 0; }
    const int idx = p.uiNoteIdx.exchange (-1);
    if (idx >= 0 && idx < 256) flash[idx] = 1.f;
    for (auto& x : flash) x *= 0.86f;
}

static juce::String noteName (int m) { return ut::keyNames[m % 12] + juce::String (m / 12 - 1); }

void Roll::paint (juce::Graphics& g)
{
    static const juce::String D3 = juce::String::fromUTF8 ("   \xc2\xb7   ");
    auto b = getLocalBounds().toFloat();
    g.setColour (col::panel); g.fillRoundedRectangle (b, 16);
    g.setColour (col::line); g.drawRoundedRectangle (b.reduced (0.5f), 16, 1);

    const int bars = p.bars(), steps = bars * 16, root = p.rootMidi(), sc = p.scale();
    const float sw = p.swing();
    // header
    g.setColour (col::lime); g.setFont (f.b (13)); g.drawText ("PATTERN", 20, 14, 90, 18, juce::Justification::left);
    g.setColour (col::grey); g.setFont (f.m (13));
    g.drawText (juce::String ((int) pat.size()) + " notes" + D3 + ut::keyNames[(int) p.p ("key")] + " " + ut::scaleName (sc) + D3
                    + juce::String (bars) + (bars == 1 ? " bar" : " bars") + D3 + "swing " + juce::String (juce::roundToInt (sw * 100)) + "%",
                100, 14, 440, 18, juce::Justification::left);
    g.setColour (col::faint); g.setFont (f.n (12));
    g.drawText (ut::styleName ((int) p.p ("style")), (int) b.getWidth() - 160, 14, 140, 18, juce::Justification::right);

    // pitch range
    std::vector<int> mids; for (auto& n : pat) mids.push_back (root + ut::degToSemi (n.deg, sc));
    int lo = root - 3, hi = root + 14;
    for (int m : mids) { lo = std::min (lo, m - 2); hi = std::max (hi, m + 2); }
    const int rows = hi - lo + 1;
    const float gx = 58, gy = 66, gw = b.getWidth() - gx - 18, velH = 64, gh = b.getHeight() - gy - velH - 46;
    const float rh = gh / rows, cw = gw / steps;
    auto xOf = [&] (double step) { const int s = (int) std::floor (step); return gx + (float) ((step + ((s % 2) ? (sw - 0.5) * 2.0 : 0.0)) * cw); };

    // kick row
    g.setColour (col::faint); g.setFont (f.s (10)); g.drawText ("KICK", 8, (int) gy - 22, 46, 14, juce::Justification::right);
    const bool kickOn = p.p ("kick") > 0.5f;
    for (int bt = 0; bt < bars * 4; ++bt)
    {
        const float x = gx + bt * 4 * cw + cw * 0.5f;
        g.setColour (kickOn ? col::limeDim : col::line); g.fillEllipse (x - 4.5f, gy - 19, 9, 9);
    }
    // lanes
    const int scalePcs[7] = { ut::scaleSteps (sc)[0], ut::scaleSteps (sc)[1], ut::scaleSteps (sc)[2], ut::scaleSteps (sc)[3], ut::scaleSteps (sc)[4], ut::scaleSteps (sc)[5], ut::scaleSteps (sc)[6] };
    for (int r = 0; r < rows; ++r)
    {
        const int m = hi - r; const int rel = ((m - root) % 12 + 12) % 12;
        bool inScale = false; for (int pc : scalePcs) inScale |= pc == rel;
        const float y = gy + r * rh;
        g.setColour (rel == 0 ? juce::Colour (0xff1d2216) : (inScale ? col::lane : juce::Colour (0xff111215)));
        g.fillRect (gx, y + 0.5f, gw, rh - 1);
        if (rel == 0 || (inScale && rh > 13))
        {
            g.setColour (rel == 0 ? col::lime : col::faint); g.setFont (rel == 0 ? f.s (11) : f.m (10));
            g.drawText (noteName (m), 4, (int) y, (int) gx - 10, (int) rh, juce::Justification::right);
        }
    }
    for (int s = 0; s <= steps; ++s)
    {
        const float x = gx + s * cw;
        g.setColour (s % 16 == 0 ? col::faint : (s % 4 == 0 ? col::line : juce::Colour (0xff1b1d22)));
        g.drawVerticalLine ((int) x, gy, gy + gh);
        if (s % 4 == 0 && s < steps)
        {
            g.setColour (s % 16 == 0 ? col::grey : col::faint); g.setFont (f.n (11));
            g.drawText (juce::String (s / 16 + 1) + (s % 16 ? "." + juce::String (s % 16 / 4 + 1) : ""), (int) x + 4, (int) (gy + gh + velH + 12), 40, 14, juce::Justification::left);
        }
    }
    // slides
    for (size_t i = 1; i < pat.size(); ++i)
        if (pat[i].slide)
        {
            const auto& a = pat[i - 1]; const auto& c = pat[i];
            const float x0 = xOf (a.step + a.len) - 2, y0 = gy + (hi - mids[i - 1]) * rh + rh * 0.5f;
            const float x1 = xOf (c.step) + 2, y1 = gy + (hi - mids[i]) * rh + rh * 0.5f;
            juce::Path pth; pth.startNewSubPath (x0 - 6, y0); pth.cubicTo (x0 + 4, y0, x1 - 4, y1, x1 + 6, y1);
            g.setColour (col::cream.withAlpha (0.7f)); g.strokePath (pth, juce::PathStrokeType (2.f));
        }
    // notes
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& n = pat[i];
        const float x0 = xOf (n.step) + 1.5f, x1 = xOf (n.step + n.len) - 1.5f;
        const float y = gy + (hi - mids[i]) * rh + 1.5f;
        juce::Rectangle<float> r (x0, y, std::max (6.f, x1 - x0), rh - 3);
        const float fl = i < 256 ? flash[i] : 0.f, v = n.vel / 127.f;
        if (fl > 0.05f)
        {
            g.setColour (col::lime.withAlpha (0.35f * fl)); g.fillRoundedRectangle (r.expanded (6 * fl), 7);
        }
        if (n.ghost)
        {
            g.setColour (col::lime.withAlpha (0.18f + 0.5f * fl)); g.fillRoundedRectangle (r, 4);
            g.setColour (col::lime.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1.2f);
            continue;
        }
        g.setGradientFill (juce::ColourGradient (col::lime.interpolatedWith (col::cream, fl).withMultipliedBrightness (0.75f + 0.25f * v), r.getX(), r.getY(),
                                                 col::limeDim.interpolatedWith (col::lime, 0.4f + 0.6f * fl), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4);
        g.setColour (juce::Colours::white.withAlpha (0.25f + 0.5f * fl)); g.fillRoundedRectangle (r.withHeight (2).reduced (2, 0), 1);
    }
    // velocity lane
    const float vy = gy + gh + 8;
    g.setColour (juce::Colour (0xff111215)); g.fillRoundedRectangle (gx, vy, gw, velH - 4, 6);
    g.setColour (col::faint); g.setFont (f.s (10)); g.drawText ("VEL", 8, (int) vy + 4, 46, 14, juce::Justification::right);
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& n = pat[i];
        const float x = xOf (n.step) + std::min (cw * 0.5f, 6.f), hh = (velH - 16) * n.vel / 127.f, fl = i < 256 ? flash[i] : 0.f;
        g.setColour (col::lime.interpolatedWith (col::cream, fl).withAlpha (0.55f + 0.45f * fl));
        g.fillRect (x - 1.5f, vy + velH - 8 - hh, 3.f, hh);
        g.fillEllipse (x - 4, vy + velH - 8 - hh - 4, 8, 8);
    }
    // playhead
    const float beat = p.uiBeat.load();
    if (beat >= 0)
    {
        const float x = gx + beat * 4 * cw;
        g.setColour (col::lime.withAlpha (0.18f)); g.fillRect (x - 5, gy - 6, 10.f, gh + velH + 10);
        g.setColour (col::lime); g.fillRect (x - 1, gy - 6, 2.f, gh + velH + 10);
    }
    if (pat.empty()) { g.setColour (col::grey); g.setFont (f.s (16)); g.drawText ("hit GENERATE", getLocalBounds(), juce::Justification::centred); }
}

// ------------------------------------------------------------------ drag tile
void DragTile::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (isMouseOver() ? juce::Colour (0xff1d2216) : col::panel2); g.fillRoundedRectangle (r, 10);
    juce::Path border; border.addRoundedRectangle (r, 10);
    juce::Path dashed; const float d[] = { 5, 4 }; juce::PathStrokeType (1.4f).createDashedStroke (dashed, border, d, 2);
    g.setColour (col::lime.withAlpha (0.8f)); g.fillPath (dashed);
    // tiny pattern preview
    auto pat = p.getPattern(); const int steps = p.bars() * 16;
    const float px = r.getX() + 12, pw = 28, py = r.getCentreY() - 9;
    int lo = 99, hi = -99; for (auto& n : pat) { lo = std::min (lo, n.deg); hi = std::max (hi, n.deg); }
    for (auto& n : pat)
    {
        const float x = px + pw * n.step / steps, y = py + (hi > lo ? 16.f * (hi - n.deg) / (hi - lo) : 8.f);
        g.setColour (col::lime); g.fillRect (x, y, std::max (1.5f, pw * n.len / steps), 2.f);
    }
    g.setColour (col::text); g.setFont (f.b (13)); g.drawText ("DRAG MIDI", (int) px + 36, (int) r.getY() + 9, 90, 16, juce::Justification::left);
    g.setColour (col::faint); g.setFont (f.n (10.5f));
    g.drawText (juce::String (p.bars()) + " bar" + (p.bars() > 1 ? "s" : "") + juce::String::fromUTF8 (" \xc2\xb7 ") + juce::String (juce::roundToInt (p.bpm.load())) + " bpm",
                (int) px + 36, (int) r.getY() + 26, (int) r.getWidth() - 50, 14, juce::Justification::left);
}

void DragTile::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Undertow");
    dir.createDirectory();
    auto f = dir.getChildFile (p.suggestedFileName());
    if (p.writeMidi (f)) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

// ------------------------------------------------------------------ editor
UndertowEditor::UndertowEditor (UndertowProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (roll);
    for (int i = 0; i < ut::NumStyles; ++i)
    {
        auto c = std::make_unique<Chip> (fonts, ut::styleName (i));
        c->sub = juce::String (i + 1).paddedLeft ('0', 2);
        c->onClick = [this, i] {
            auto* prm = proc.apvts.getParameter ("style");
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i)); prm->endChangeGesture();
        };
        addAndMakeVisible (*c); styles.push_back (std::move (c));
    }
    auto& st = proc.apvts;
    const std::pair<const char*, const char*> ks[] = { { "density", "Density" }, { "movement", "Movement" }, { "variation", "Variation" }, { "gate", "Length" },
                                                         { "swing", "Swing" }, { "glide", "Glide" }, { "tone", "Tone" }, { "drive", "Drive" }, { "volume", "Volume" }, { "ghosts", "Ghosts" }, { "accent", "Accent" } };
    for (auto& k : ks) { knobs.push_back (std::make_unique<Knob> (fonts, st, k.first, k.second)); addAndMakeVisible (*knobs.back()); }

    auto combo = [&] (juce::ComboBox& c, const char* id) {
        c.addItemList (st.getParameter (id)->getAllValueStrings(), 1);
        addAndMakeVisible (c);
        combos.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, c));
    };
    combo (key, "key"); combo (scale, "scale"); combo (octave, "octave"); combo (bars, "bars"); combo (sound, "sound"); combo (chords, "chords");

    for (auto* b : { &generate, &mutate, &undo, &play, &kick, &lockR, &lockP, &exportB }) addAndMakeVisible (*b);
    for (auto* b : { &kick, &lockR, &lockP }) { b->setClickingTogglesState (true); b->filled = false; }
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "kick", kick));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "lockRhythm", lockR));
    btns.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, "lockPitch", lockP));
    generate.onClick = [this] { proc.generateNew(); };
    mutate.onClick = [this] { proc.mutate(); };
    undo.onClick = [this] { proc.undo(); };
    play.onClick = [this] { proc.internalPlay = ! proc.internalPlay.load(); };
    exportB.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Export MIDI", juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.suggestedFileName()), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f != juce::File()) proc.writeMidi (f.withFileExtension ("mid")); });
    };
    addAndMakeVisible (drag);
    setSize (W, H);
    startTimerHz (60);
}

UndertowEditor::~UndertowEditor() { setLookAndFeel (nullptr); }

void UndertowEditor::timerCallback()
{
    roll.refresh(); roll.repaint();
    const int s = (int) proc.p ("style");
    if (s != lastStyle) { for (int i = 0; i < (int) styles.size(); ++i) styles[(size_t) i]->setToggleState (i == s, juce::dontSendNotification); lastStyle = s; repaint (16, 380, 250, 50); }
    play.setToggleState (proc.internalPlay.load() || proc.uiBeat.load() >= 0, juce::dontSendNotification);
    undo.setEnabled (proc.canUndo()); undo.setAlpha (proc.canUndo() ? 1.f : 0.4f);
    static int tick = 0; if (++tick % 10 == 0) { drag.repaint(); repaint (W - 250, 0, 250, 80); repaint (60, 690, 130, 60); }
    repaint (900, 580, 250, 80);
}

void UndertowEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    // subtle top glow
    g.setGradientFill (juce::ColourGradient (col::lime.withAlpha (0.06f), W * 0.3f, 0, col::bg.withAlpha (0.f), W * 0.3f, 260, false));
    g.fillRect (0, 0, W, 260);
    // header
    // logo: three stacked waves
    for (int k = 0; k < 3; ++k)
    {
        juce::Path w; const float y0 = 26.f + k * 9.f, amp = 4.f - k;
        for (int i = 0; i <= 40; ++i) { const float x = 22 + i * 0.85f, y = y0 + amp * std::sin (i * 0.33f + k); i ? w.lineTo (x, y) : w.startNewSubPath (x, y); }
        g.setColour (col::lime.withAlpha (1.f - k * 0.3f)); g.strokePath (w, juce::PathStrokeType (3.2f - k * 0.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (col::text); g.setFont (fonts.b (34)); g.drawText ("UNDERTOW", 66, 12, 260, 38, juce::Justification::left);
    g.setColour (col::lime); g.setFont (fonts.s (11.5f));
    g.drawText ("B A S S L I N E   G E N E R A T O R", 68, 50, 300, 14, juce::Justification::left);
    g.setColour (col::faint); g.setFont (fonts.s (10.5f));
    for (auto [x, w, t] : { std::tuple<int, int, const char*> { 470, 70, "KEY" }, { 548, 130, "SCALE" }, { 686, 64, "OCT" }, { 758, 92, "BARS" }, { 858, 150, "CHORDS" } })
        g.drawText (t, x + 2, 14, w, 12, juce::Justification::left);
    g.setColour (col::faint); g.drawText ("by", W - 140, 22, 30, 14, juce::Justification::right);
    g.setColour (col::text); g.setFont (fonts.b (15)); g.drawText ("TOKEN WISE", W - 108, 19, 90, 20, juce::Justification::right);
    g.setColour (col::faint); g.setFont (fonts.n (10.5f)); g.drawText ("v1.0", W - 108, 40, 90, 14, juce::Justification::right);

    // section labels
    auto lab = [&] (const char* t, int x, int y) { g.setColour (col::grey); g.setFont (fonts.b (11.5f)); g.drawText (t, x, y, 200, 14, juce::Justification::left); };
    lab ("STYLE", 22, 92); lab ("SHAPE", 22, 432); lab ("FEEL", 912, 92); lab ("SOUND", 912, 320);
    // style blurb
    g.setColour (col::grey); g.setFont (fonts.m (12.5f));
    g.drawFittedText (ut::styleBlurb ((int) proc.p ("style")), 22, 390, 236, 36, juce::Justification::topLeft, 2);
    // side panels
    g.setColour (col::panel.withAlpha (0.6f));
    // bottom bar
    g.setColour (col::panel); g.fillRoundedRectangle (12, 690, W - 24, 58, 16);
    g.setColour (col::line); g.drawRoundedRectangle (12.5f, 690.5f, W - 25, 57, 16, 1);
    g.setColour (col::text); g.setFont (fonts.n (17));
    g.drawText (juce::String (juce::roundToInt (proc.bpm.load())), 78, 702, 60, 20, juce::Justification::left);
    g.setColour (col::faint); g.setFont (fonts.s (10)); g.drawText (juce::String::fromUTF8 ("BPM \xc2\xb7 HOST"), 78, 723, 90, 12, juce::Justification::left);
    g.setColour (col::faint); g.setFont (fonts.s (10.5f)); g.drawText ("LOCK", 268, 712, 40, 14, juce::Justification::left);
    // scope
    {
        const juce::Rectangle<float> sc (912, 588, 228, 64);
        g.setColour (col::panel); g.fillRoundedRectangle (sc, 10);
        g.setColour (col::line); g.drawRoundedRectangle (sc.reduced (0.5f), 10, 1);
        g.setColour (col::line); g.drawHorizontalLine ((int) sc.getCentreY(), sc.getX() + 8, sc.getRight() - 8);
        const int w0 = proc.scopeW.load();
        // find a rising zero crossing so the wave stands still
        int start = (w0 - 400 + 1024) & 1023;
        for (int k = 0; k < 200; ++k) { const int a = (start + k) & 1023, b2 = (a + 1) & 1023; if (proc.scope[(size_t) a] <= 0 && proc.scope[(size_t) b2] > 0) { start = a; break; } }
        juce::Path wave; const int N = 200;
        float mx = 0.05f; for (int k = 0; k < N; ++k) mx = std::max (mx, std::abs (proc.scope[(size_t) ((start + k) & 1023)]));
        for (int k = 0; k < N; ++k)
        {
            const float x = sc.getX() + 8 + (sc.getWidth() - 16) * k / (N - 1), y = sc.getCentreY() - proc.scope[(size_t) ((start + k) & 1023)] / mx * (sc.getHeight() * 0.38f);
            k ? wave.lineTo (x, y) : wave.startNewSubPath (x, y);
        }
        g.setColour (col::lime.withAlpha (0.25f)); g.strokePath (wave, juce::PathStrokeType (5.f, juce::PathStrokeType::curved));
        g.setColour (col::lime); g.strokePath (wave, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        g.setColour (col::faint); g.setFont (fonts.s (10)); g.drawText ("OUT", (int) sc.getX() + 10, (int) sc.getY() + 6, 60, 12, juce::Justification::left);
    }
    g.setColour (col::faint); g.setFont (fonts.m (11));
    g.drawFittedText ("MIDI goes out too: route it to Serpent or any synth.", 912, 658, 228, 26, juce::Justification::topLeft, 2);
}

void UndertowEditor::resized()
{
    for (int i = 0; i < (int) styles.size(); ++i) styles[(size_t) i]->setBounds (22 + (i % 2) * 122, 112 + (i / 2) * 46, 114, 40);
    knobs[0]->setBounds (22, 450, 112, 86); knobs[1]->setBounds (144, 450, 112, 86);
    knobs[2]->setBounds (22, 540, 112, 86); knobs[3]->setBounds (144, 540, 112, 86);
    roll.setBounds (278, 84, 616, 596);
    knobs[4]->setBounds (912, 112, 108, 96); knobs[5]->setBounds (1030, 112, 108, 96);
    knobs[9]->setBounds (912, 212, 108, 96); knobs[10]->setBounds (1030, 212, 108, 96);
    sound.setBounds (912, 340, 228, 34);
    knobs[6]->setBounds (912, 382, 108, 96); knobs[7]->setBounds (1030, 382, 108, 96);
    knobs[8]->setBounds (912, 482, 108, 96);
    key.setBounds (470, 30, 70, 32); scale.setBounds (548, 30, 130, 32); octave.setBounds (686, 30, 64, 32); bars.setBounds (758, 30, 92, 32); chords.setBounds (858, 30, 150, 32);
    play.setBounds (24, 697, 44, 44);
    kick.setBounds (168, 701, 84, 36);
    lockR.setBounds (306, 701, 90, 36); lockP.setBounds (402, 701, 76, 36);
    generate.setBounds (500, 696, 232, 46);
    mutate.setBounds (744, 701, 108, 36); undo.setBounds (860, 701, 64, 36);
    drag.setBounds (932, 699, 134, 40); exportB.setBounds (1072, 701, 68, 36);
}

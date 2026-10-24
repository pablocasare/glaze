#include "PluginEditor.h"
#include "BinaryData.h"

using namespace ui;
using juce::Rectangle;
using juce::Colour;

// ============================================================================
// Fonts
Fonts& Fonts::get()
{
    static Fonts f = []
    {
        Fonts x;
        x.logo     = juce::Typeface::createSystemTypefaceFor (BinaryData::RighteousRegular_ttf, BinaryData::RighteousRegular_ttfSize);
        x.mono     = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
        x.monoBold = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
        return x;
    }();
    return f;
}
juce::Font Fonts::logoFont (float h) const { return juce::Font (juce::FontOptions().withTypeface (logo).withHeight (h)); }
juce::Font Fonts::monoFont (float h, bool bold) const { return juce::Font (juce::FontOptions().withTypeface (bold ? monoBold : mono).withHeight (h)); }

static juce::Font mono (float h, bool bold = false) { return Fonts::get().monoFont (h, bold); }

juce::String ui::formatValue (const juce::RangedAudioParameter& p, float v)
{
    if (dynamic_cast<const juce::AudioParameterChoice*> (&p) != nullptr)
        return p.getText (p.convertTo0to1 (v), 32);
    const auto u = p.getLabel();
    if (u == "%")  return juce::String (juce::roundToInt (v)) + " %";
    if (u == "dB") return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB";
    if (u == "Hz") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    if (u == "ms") return v < 10.0f ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms";
    if (u == ":1") return juce::String (v, 1) + ":1";
    if (u == "st") return (v > 0.05f ? "+" : "") + juce::String (v, std::abs (v - std::round (v)) < 0.05f ? 0 : 1) + " st";
    if (u == "ct") return juce::String (juce::roundToInt (v)) + " ct";
    return (v > 0.5f ? "+" : "") + juce::String (juce::roundToInt (v));
}

// ============================================================================
// Look and feel
GlazeLookAndFeel::GlazeLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, col::shell);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::arrowColourId, col::accent);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, col::bg);
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::highlightColourId, col::accent.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, col::pink);
    setColour (juce::TextButton::buttonColourId, col::knob);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TooltipWindow::backgroundColourId, col::shell);
    setColour (juce::TooltipWindow::textColourId, col::text);
}

void GlazeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = std::min (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float ang = start + pos * (end - start);
    const bool hot = s.isMouseOverOrDragging();

    // dotted scale
    const int dots = 25;
    for (int i = 0; i < dots; ++i)
    {
        const float t = (float) i / (dots - 1);
        const float a = start + t * (end - start);
        const auto pt = c.getPointOnCircumference (r * 0.95f, a);
        const bool lit = t <= pos + 1e-4f;
        g.setColour (lit ? col::accent.interpolatedWith (col::pink, t * 0.7f) : col::dim.withAlpha (0.35f));
        const float ds = lit ? 2.6f : 1.8f;
        g.fillEllipse (pt.x - ds * 0.5f, pt.y - ds * 0.5f, ds, ds);
    }

    // body
    const float br = r * 0.72f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - br, c.y - br + 3.0f, br * 2, br * 2);
    juce::ColourGradient body (Colour (0xff463863), c.x - br * 0.6f, c.y - br, Colour (0xff17111f), c.x + br * 0.5f, c.y + br, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (hot ? col::accent.withAlpha (0.8f) : col::edge);
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.2f);
    // inner cap
    const float cr = br * 0.62f;
    juce::ColourGradient cap (Colour (0xff3a2e52), c.x, c.y - cr, Colour (0xff221a31), c.x, c.y + cr, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);

    // pointer
    const auto p1 = c.getPointOnCircumference (br * 0.25f, ang);
    const auto p2 = c.getPointOnCircumference (br * 0.88f, ang);
    g.setColour (col::pink.withAlpha (0.35f));
    g.drawLine ({ p1, p2 }, 5.0f);
    g.setColour (col::accentHi);
    g.drawLine ({ p1, p2 }, 2.2f);
}

void GlazeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff160f24));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver() ? col::accent : col::edge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (col::accent);
    g.fillPath (arrow);
}
juce::Font GlazeLookAndFeel::getComboBoxFont (juce::ComboBox&) { return mono (12.0f, true); }
void GlazeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 0, box.getWidth() - 24, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}
juce::Font GlazeLookAndFeel::getPopupMenuFont() { return mono (13.0f); }
void GlazeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::shell);
    g.setColour (col::edge);
    g.drawRect (0, 0, w, h);
}
juce::Label* GlazeLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (11.5f, true));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, col::text);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}
juce::Font GlazeLookAndFeel::getLabelFont (juce::Label&) { return mono (11.5f, true); }

// ============================================================================
// Controls
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& lbl, bool)
    : label (lbl.toUpperCase())
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 16);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id))
    {
        slider.textFromValueFunction = [p] (double v) { return formatValue (*p, (float) v); };
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        slider.setTooltip (p->getName (64) + "  (double-click to reset)");
        slider.updateText();
    }
}
void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedTop (14)); }
void Knob::paint (juce::Graphics& g)
{
    g.setFont (mono (9.5f, true));
    g.setColour (col::dim);
    g.drawText (label, getLocalBounds().removeFromTop (14), juce::Justification::centred);
}

PillToggle::PillToggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, juce::String on, juce::String off)
    : onT (std::move (on)), offT (std::move (off))
{
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
}
void PillToggle::paintButton (juce::Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    g.setColour (on ? col::accent.withAlpha (0.28f) : Colour (0xff160f24));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? col::accent : (over ? col::dim : col::edge));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    // led
    const float d = 6.0f;
    g.setColour (on ? col::pink : col::dim.withAlpha (0.4f));
    g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - d * 0.5f, d, d);
    g.setColour (on ? col::text : col::dim);
    g.setFont (mono (10.0f, true));
    g.drawText (on ? onT : offT, r.withTrimmedLeft (14.0f), juce::Justification::centred);
}

PowerButton::PowerButton (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
    setTooltip ("Module on / off");
}
void PowerButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();
    g.setColour (on ? col::accent.withAlpha (0.25f) : Colour (0xff160f24));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? col::accent : (over ? col::dim : col::edge));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    const auto c = r.getCentre();
    const float rad = r.getHeight() * 0.26f;
    juce::Path p;
    p.addCentredArc (c.x, c.y + 1, rad, rad, 0, juce::degreesToRadians (35.0f), juce::degreesToRadians (325.0f), true);
    const Colour ic = on ? col::pink : col::dim;
    g.setColour (ic);
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine (c.x, c.y - rad - 1, c.x, c.y + 1, 1.8f);
}

ParamCombo::ParamCombo (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        addItemList (p->choices, 1);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *this);
}

// ============================================================================
// Screen base
void Screen::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float rad = 14.0f;

    // bezel
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 3), rad);

    juce::Path clip; clip.addRoundedRectangle (r, rad);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (clip);
        juce::ColourGradient grad (col::screenA, r.getX(), r.getY(), col::screenB, r.getRight(), r.getBottom(), false);
        grad.addColour (0.55, Colour (0xff3410a0));
        g.setGradientFill (grad);
        g.fillRect (r);
        // glow blob
        juce::ColourGradient blob (col::pink.withAlpha (0.22f), r.getCentreX(), r.getBottom(), juce::Colours::transparentBlack, r.getCentreX(), r.getY() + r.getHeight() * 0.2f, true);
        g.setGradientFill (blob);
        g.fillRect (r);

        drawContent (g, r.reduced (12.0f, 10.0f));

        // scanlines + vignette
        g.setColour (juce::Colours::black.withAlpha (0.06f));
        for (float y = r.getY(); y < r.getBottom(); y += 3.0f) g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
        juce::ColourGradient vig (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (0.45f), r.getX(), r.getY(), true);
        g.setGradientFill (vig);
        g.fillRect (r);
        // gloss
        juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.35f, false);
        g.setGradientFill (gloss);
        g.fillRect (r.withHeight (r.getHeight() * 0.35f));
    }
    g.setColour (col::accent.withAlpha (0.55f));
    g.drawRoundedRectangle (r, rad, 1.3f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r.expanded (2.0f), rad + 2, 2.0f);
}

void Screen::caption (juce::Graphics& g, Rectangle<float> r, const juce::String& left, const juce::String& right)
{
    g.setFont (mono (11.0f, true));
    g.setColour (col::text.withAlpha (0.92f));
    g.drawText (left, r.removeFromTop (16), juce::Justification::centredLeft);
    if (right.isNotEmpty())
    {
        g.setColour (col::accentHi.withAlpha (0.8f));
        g.drawText (right, r.withY (r.getY() - 16).withHeight (16), juce::Justification::centredRight);
    }
}

float ui::freqToX (float hz, Rectangle<float> r)
{
    const float t = std::log (hz / 20.0f) / std::log (20000.0f / 20.0f);
    return r.getX() + t * r.getWidth();
}
float ui::xToFreq (float x, Rectangle<float> r)
{
    const float t = (x - r.getX()) / r.getWidth();
    return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, t));
}

static void drawFreqGrid (juce::Graphics& g, Rectangle<float> r)
{
    g.setFont (mono (9.0f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = freqToX (f, r);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        g.setColour (col::accentHi.withAlpha (0.5f));
        g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), (int) x - 15, (int) r.getBottom() - 12, 30, 12, juce::Justification::centred);
    }
}

// ============================================================================
// Spectrum
Spectrum::Spectrum() : data ((size_t) size * 2, 0.0f), smooth ((size_t) size / 2, -120.0f) {}
void Spectrum::update (const glaze::ScopeBuffer& sb, double s)
{
    sr = s;
    std::vector<float> l ((size_t) size), r ((size_t) size);
    sb.copyLast (size, l.data(), r.data());
    for (int i = 0; i < size; ++i) data[(size_t) i] = 0.5f * (l[(size_t) i] + r[(size_t) i]);
    window.multiplyWithWindowingTable (data.data(), (size_t) size);
    std::fill (data.begin() + size, data.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (data.data());
    for (int i = 0; i < size / 2; ++i)
    {
        const float db = glaze::gainToDb (data[(size_t) i] * 4.0f / size);
        auto& s2 = smooth[(size_t) i];
        s2 = db > s2 ? s2 + (db - s2) * 0.6f : s2 + (db - s2) * 0.15f;
    }
}
float Spectrum::dbAt (float hz) const
{
    const float bin = hz / (float) sr * size;
    const int b = juce::jlimit (1, size / 2 - 2, (int) bin);
    // max over a small neighbourhood so high bands don't look sparse
    const int span = juce::jmax (1, (int) (bin * 0.03f));
    float m = -140.0f;
    for (int i = juce::jmax (1, b - span); i <= juce::jmin (size / 2 - 1, b + span); ++i) m = std::max (m, smooth[(size_t) i]);
    return m;
}
void Spectrum::draw (juce::Graphics& g, Rectangle<float> r, Colour c, float minDb, float maxDb) const
{
    juce::Path p;
    p.startNewSubPath (r.getX(), r.getBottom());
    for (float x = r.getX(); x <= r.getRight(); x += 2.0f)
    {
        // pink-ish tilt (+4.5 dB/oct) so vocals read flat
        const float f = xToFreq (x, r);
        const float db = dbAt (f) + 4.5f * std::log2 (f / 1000.0f);
        const float y = juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, r.getBottom(), r.getY());
        p.lineTo (x, y);
    }
    p.lineTo (r.getRight(), r.getBottom());
    p.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.55f), r.getX(), r.getY(), c.withAlpha (0.05f), r.getX(), r.getBottom(), false));
    g.fillPath (p);
    g.setColour (c.withAlpha (0.8f));
    g.strokePath (p, juce::PathStrokeType (1.0f));
}

// ============================================================================
// Screens
namespace ui
{
static juce::String noteName (int midi) { return glaze::keyNames[((midi % 12) + 12) % 12] + juce::String (midi / 12 - 1); }

static juce::Path logoPath (float h)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (Fonts::get().logoFont (h), "glaze", 0, 0);
    juce::Path p; ga.createPath (p);
    return p;
}

class HomeScreen : public Screen
{
public:
    using Screen::Screen;
    History wave { 140, 0.0f };
    float phase = 0;
    void tick() override
    {
        float buf[1470];
        const int n = (int) std::min (1470.0, proc.getSampleRateSafe() / 30.0);
        proc.inScope.copyLast (n, buf, nullptr);
        float pk = 0; for (int i = 0; i < n; ++i) pk = std::max (pk, std::abs (buf[i]));
        wave.push (pk);
        phase += 0.04f;
    }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        // waveform strip
        auto strip = r.removeFromTop (58);
        const int N = wave.size();
        const float bw = strip.getWidth() / N;
        for (int i = 0; i < N; ++i)
        {
            const float v = std::pow (juce::jlimit (0.0f, 1.0f, wave[i] * 1.6f), 0.6f);
            const float hh = std::max (1.0f, v * strip.getHeight() * 0.5f);
            const float x = strip.getX() + i * bw;
            g.setColour (col::pink.interpolatedWith (col::accentHi, (float) i / N).withAlpha (0.35f + 0.65f * (float) i / N));
            g.fillRect (x, strip.getCentreY() - hh, std::max (1.0f, bw - 1.0f), hh * 2);
        }
        r.removeFromTop (4);
        const auto preset = proc.getProgramName (proc.getCurrentProgram()).toUpperCase();
        g.setFont (mono (13.0f, true));
        g.setColour (col::text);
        g.drawText ("NOW GLAZING:  < " + preset + " >", r.removeFromTop (18), juce::Justification::centred);
        g.setFont (mono (10.0f));
        g.setColour (col::accentHi.withAlpha (0.65f));
        g.drawText ("KEY / SCALE / PRESET  -  OR JUST SING", r.removeFromTop (14), juce::Justification::centred);
        r.removeFromTop (34); // key/scale combos sit here

        // logo
        auto logoArea = r;
        auto path = logoPath (100.0f);
        path.applyTransform (juce::AffineTransform::shear (-0.22f, 0.0f));
        auto pb = path.getBounds();
        const float scale = std::min (logoArea.getWidth() * 0.62f / pb.getWidth(), logoArea.getHeight() * 0.78f / pb.getHeight());
        path.applyTransform (juce::AffineTransform::translation (-pb.getCentreX(), -pb.getCentreY()).scaled (scale)
                                 .translated (logoArea.getCentreX(), logoArea.getCentreY() - 4));
        pb = path.getBounds();

        // orbit ring (back half)
        const auto rc = pb.getCentre();
        const float rw = pb.getWidth() * 0.66f, rh = pb.getHeight() * 0.22f;
        auto ringT = juce::AffineTransform::rotation (-0.16f, rc.x, rc.y);
        juce::Path ring; ring.addEllipse (rc.x - rw, rc.y - rh + pb.getHeight() * 0.18f, rw * 2, rh * 2);
        ring.applyTransform (ringT);
        g.setColour (col::pink.withAlpha (0.35f));
        g.strokePath (ring, juce::PathStrokeType (5.0f));

        // drips hanging under the letters
        juce::Path drips;
        const float dripXs[] { 0.12f, 0.43f, 0.71f, 0.93f };
        for (int i = 0; i < 4; ++i)
        {
            const float len = (14.0f + 9.0f * std::sin (phase + i * 1.7f)) * scale * 0.9f + 6.0f;
            const float x = pb.getX() + pb.getWidth() * dripXs[i];
            const float y = pb.getBottom() - pb.getHeight() * 0.18f;
            const float w = 5.5f + (i % 2) * 2.0f;
            drips.addRoundedRectangle (x - w * 0.5f, y, w, len, w * 0.5f);
            drips.addEllipse (x - w * 0.75f, y + len - w * 0.9f, w * 1.5f, w * 1.5f);
        }
        juce::DropShadow (col::pink.withAlpha (0.9f), 26, {}).drawForPath (g, path);
        g.setGradientFill (juce::ColourGradient (col::accentHi, 0, pb.getY(), col::pink, 0, pb.getBottom() + 30, false));
        g.fillPath (drips);
        juce::ColourGradient fill (juce::Colours::white, pb.getX(), pb.getY(), col::pink, pb.getX(), pb.getBottom(), false);
        fill.addColour (0.45, col::accentHi);
        g.setGradientFill (fill);
        g.fillPath (path);
        g.setColour (Colour (0xff2a0a6a));
        g.strokePath (path, juce::PathStrokeType (2.0f));
        // highlight
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.strokePath (path, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (-1.2f, -1.6f));

        // front half of ring
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (Rectangle<int> ((int) (rc.x - rw * 1.2f), (int) (rc.y + pb.getHeight() * 0.18f), (int) (rw * 2.4f), (int) (rh * 3)));
        g.setColour (col::accentHi.withAlpha (0.85f));
        g.strokePath (ring, juce::PathStrokeType (3.0f));
    }
};

class TuneScreen : public Screen
{
public:
    using Screen::Screen;
    History det { 220, -1.0f }, out { 220, -1.0f };
    float centre = 60.0f;
    int silentTicks = 999;
    void tick() override
    {
        auto& t = proc.getTuner();
        const float d = t.uiDetectedMidi.load(), o = t.uiOutMidi.load();
        det.push (d); out.push (o);
        if (d > 0) { silentTicks = 0; centre += (std::round (d) - centre) * 0.08f; }
        else ++silentTicks;
    }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        auto& t = proc.getTuner();
        const float d = t.uiDetectedMidi.load(), tg = t.uiTargetMidi.load(), o = t.uiOutMidi.load();
        if (silentTicks > 20 || d < 0)
            caption (g, r, param ("tuneOn") > 0.5f ? "LISTENING..." : "TUNE BYPASSED", "GREY = IN   PURPLE = TUNED");
        else
        {
            const int c = juce::roundToInt ((d - std::round (d)) * 100.0f);
            caption (g, r, noteName ((int) std::round (d)) + "  " + (c >= 0 ? "+" : "") + juce::String (c) + " ct  ->  "
                               + (tg >= 0 ? noteName ((int) tg) : juce::String ("--")) + "  (" + juce::String (o - std::round (o) >= 0 ? "+" : "")
                               + juce::String (juce::roundToInt ((o - std::round (o)) * 100)) + " ct)",
                     "GREY = IN   PURPLE = TUNED");
        }
        r.removeFromTop (20);
        auto gutter = r.removeFromLeft (34);
        const float range = 7.5f;
        const float lo = centre - range, hi = centre + range;
        auto yOf = [&] (float m) { return juce::jmap (m, lo, hi, r.getBottom(), r.getY()); };
        const int mask = glaze::scaleMask ((int) param ("scale"));
        const int key = (int) param ("key");
        g.setFont (mono (9.5f, true));
        for (int n = (int) std::ceil (lo); n <= (int) std::floor (hi); ++n)
        {
            const bool inScale = (mask >> (((n - key) % 12 + 12) % 12)) & 1;
            const float y = yOf ((float) n);
            if (inScale)
            {
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.fillRect (r.getX(), y - 0.5f * r.getHeight() / (2 * range), r.getWidth(), r.getHeight() / (2 * range));
            }
            g.setColour (juce::Colours::white.withAlpha (inScale ? 0.12f : 0.04f));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
            g.setColour (inScale ? col::text.withAlpha (0.85f) : col::dim.withAlpha (0.6f));
            g.drawText (noteName (n), gutter.withY (y - 6).withHeight (12), juce::Justification::centredLeft);
        }
        auto plot = [&] (const History& h, Colour c, float th, bool glow)
        {
            juce::Path p; bool pen = false;
            const float dx = r.getWidth() / (h.size() - 1);
            for (int i = 0; i < h.size(); ++i)
            {
                const float v = h[i];
                if (v < 0) { pen = false; continue; }
                const float x = r.getX() + i * dx, y = juce::jlimit (r.getY(), r.getBottom(), yOf (v));
                if (! pen) { p.startNewSubPath (x, y); pen = true; } else p.lineTo (x, y);
            }
            if (glow) { g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (th * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
            g.setColour (c);
            g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        plot (det, juce::Colours::white.withAlpha (0.45f), 1.4f, false);
        plot (out, col::pink.interpolatedWith (col::accentHi, 0.3f), 2.4f, true);
    }
};

class GateScreen : public Screen
{
public:
    using Screen::Screen;
    History lvl { 200, -100.0f }, open { 200, 1.0f };
    void tick() override { lvl.push (proc.gateEnvDb.load()); open.push (proc.gateGainDb.load() > -1.0f ? 1.0f : 0.0f); }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const bool isOpen = open[open.size() - 1] > 0.5f;
        caption (g, r, param ("gateOn") > 0.5f ? (isOpen ? "GATE  OPEN" : "GATE  CLOSED") : "GATE  OFF", "PINK = GATE LINE");
        r.removeFromTop (20);
        auto yOf = [&] (float db) { return juce::jmap (juce::jlimit (-80.0f, 0.0f, db), -80.0f, 0.0f, r.getBottom(), r.getY()); };
        const float dx = r.getWidth() / (lvl.size() - 1);
        for (int i = 0; i < lvl.size(); ++i)
            if (open[i] < 0.5f) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRect (r.getX() + i * dx, r.getY(), dx + 0.5f, r.getHeight()); }
        juce::Path p; p.startNewSubPath (r.getX(), r.getBottom());
        for (int i = 0; i < lvl.size(); ++i) p.lineTo (r.getX() + i * dx, yOf (lvl[i]));
        p.lineTo (r.getRight(), r.getBottom()); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col::accentHi.withAlpha (0.8f), 0, r.getY(), col::accent.withAlpha (0.2f), 0, r.getBottom(), false));
        g.fillPath (p);
        const float th = param ("gateThresh");
        const float y = yOf (th);
        g.setColour (col::pink.withAlpha (0.35f)); g.fillRect (r.getX(), y - 2, r.getWidth(), 4.0f);
        g.setColour (col::pink); g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        g.setFont (mono (10.0f, true));
        g.drawText ("THRESHOLD " + juce::String (th, 1) + " dB", (int) r.getX() + 4, (int) y - 15, 200, 13, juce::Justification::left);
    }
};

class EqScreen : public Screen
{
public:
    using Screen::Screen;
    Spectrum spec;
    int drag = -1;
    Rectangle<float> plotArea;
    void tick() override { spec.update (proc.outScope, proc.getSampleRateSafe()); }

    struct Node { const char* xId; const char* yId; float fixedFreq; };
    const Node nodes[4] { { "eqLowCut", nullptr, 0 }, { "eqBodyFreq", "eqBody", 0 }, { "eqPresFreq", "eqPres", 0 }, { nullptr, "eqAir", 10000.0f } };

    float yOfDb (float db) const { return juce::jmap (db, -15.0f, 15.0f, plotArea.getBottom() - 14, plotArea.getY() + 4); }
    float dbOfY (float y) const { return juce::jmap (y, plotArea.getBottom() - 14, plotArea.getY() + 4, -15.0f, 15.0f); }
    juce::Point<float> nodePos (int i) const
    {
        const auto& n = nodes[i];
        const float f = n.xId ? param (n.xId) : n.fixedFreq;
        const float db = n.yId ? param (n.yId) : 0.0f;
        return { freqToX (f, plotArea), yOfDb (db) };
    }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        caption (g, r, "DRAG THE DOTS  |  DOUBLE-CLICK = RESET", param ("eqOn") > 0.5f ? "" : "EQ OFF");
        r.removeFromTop (18);
        plotArea = r;
        drawFreqGrid (g, r);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawHorizontalLine ((int) yOfDb (0), r.getX(), r.getRight());
        spec.draw (g, r.withTrimmedBottom (14), col::accentHi.withAlpha (0.6f), -100, -10);

        // response
        const double sr = 48000;
        glaze::Biquad b[5];
        b[0].set (glaze::Biquad::HighPass, sr, param ("eqLowCut"), 0.54f);
        b[1].set (glaze::Biquad::HighPass, sr, param ("eqLowCut"), 1.31f);
        b[2].set (glaze::Biquad::Peak, sr, param ("eqBodyFreq"), 0.8f, param ("eqBody"));
        b[3].set (glaze::Biquad::Peak, sr, param ("eqPresFreq"), 0.9f, param ("eqPres"));
        b[4].set (glaze::Biquad::HighShelf, sr, 10000.0f, 0.7f, param ("eqAir"));
        juce::Path curve;
        for (float x = r.getX(); x <= r.getRight(); x += 1.5f)
        {
            const float f = xToFreq (x, r);
            float mag = 1; for (auto& q : b) mag *= q.magnitude (f, sr);
            const float y = juce::jlimit (r.getY(), r.getBottom(), yOfDb (glaze::gainToDb (mag)));
            if (curve.isEmpty()) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }
        g.setColour (col::pink.withAlpha (0.3f));
        g.strokePath (curve, juce::PathStrokeType (6.0f));
        g.setColour (col::pink.interpolatedWith (juce::Colours::white, 0.2f));
        g.strokePath (curve, juce::PathStrokeType (2.2f));
        for (int i = 0; i < 4; ++i)
        {
            const auto p = nodePos (i);
            g.setColour (col::pink.withAlpha (0.4f)); g.fillEllipse (p.x - 9, p.y - 9, 18, 18);
            g.setColour (juce::Colours::white); g.fillEllipse (p.x - 5, p.y - 5, 10, 10);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        drag = -1; float best = 16.0f;
        for (int i = 0; i < 4; ++i) { const float d = nodePos (i).getDistanceFrom (e.position); if (d < best) { best = d; drag = i; } }
        if (drag >= 0) for (auto id : { nodes[drag].xId, nodes[drag].yId }) if (id) proc.apvts.getParameter (id)->beginChangeGesture();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto& n = nodes[drag];
        if (n.xId) { auto* p = proc.apvts.getParameter (n.xId); p->setValueNotifyingHost (p->convertTo0to1 (xToFreq (e.position.x, plotArea))); }
        if (n.yId) { auto* p = proc.apvts.getParameter (n.yId); p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-12.0f, 12.0f, dbOfY (e.position.y)))); }
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (drag >= 0) for (auto id : { nodes[drag].xId, nodes[drag].yId }) if (id) proc.apvts.getParameter (id)->endChangeGesture();
        drag = -1;
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < 4; ++i)
            if (nodePos (i).getDistanceFrom (e.position) < 16.0f)
                for (auto id : { nodes[i].xId, nodes[i].yId })
                    if (id) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->getDefaultValue()); }
    }
};

class CompScreen : public Screen
{
public:
    using Screen::Screen;
    History lvl { 200, -100.0f }, gr { 200, 0.0f };
    void tick() override { lvl.push (proc.compInDb.load()); gr.push (proc.compGrDb.load()); }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        caption (g, r, "GAIN REDUCTION  -" + juce::String (std::abs (gr[gr.size() - 1]), 1) + " dB", "RATIO " + juce::String (param ("compRatio"), 1) + ":1");
        r.removeFromTop (20);
        auto yOf = [&] (float db) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, r.getBottom(), r.getY()); };
        const float dx = r.getWidth() / (lvl.size() - 1);
        juce::Path p; p.startNewSubPath (r.getX(), r.getBottom());
        for (int i = 0; i < lvl.size(); ++i) p.lineTo (r.getX() + i * dx, yOf (lvl[i]));
        p.lineTo (r.getRight(), r.getBottom()); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col::accentHi.withAlpha (0.7f), 0, r.getY(), col::accent.withAlpha (0.15f), 0, r.getBottom(), false));
        g.fillPath (p);
        for (int i = 0; i < gr.size(); ++i)
        {
            const float hgt = juce::jlimit (0.0f, 1.0f, gr[i] / 24.0f) * r.getHeight() * 0.6f;
            g.setColour (col::pink.withAlpha (0.75f));
            g.fillRect (r.getX() + i * dx, r.getY(), dx + 0.5f, hgt);
        }
        const float th = param ("compThresh");
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawHorizontalLine ((int) yOf (th), r.getX(), r.getRight());
        g.setFont (mono (10.0f, true));
        g.drawText ("THRESHOLD " + juce::String (th, 1) + " dB", (int) r.getX() + 4, (int) yOf (th) + 2, 220, 13, juce::Justification::left);
    }
};

class DeEssScreen : public Screen
{
public:
    using Screen::Screen;
    Spectrum spec; Rectangle<float> plotArea; bool dragging = false;
    void tick() override { spec.update (proc.outScope, proc.getSampleRateSafe()); }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const float f = param ("dsFreq");
        caption (g, r, "SIBILANCE BAND  >  " + juce::String (juce::roundToInt (f)) + " Hz", "DRAG TO MOVE");
        r.removeFromTop (18);
        plotArea = r;
        drawFreqGrid (g, r);
        spec.draw (g, r.withTrimmedBottom (14), col::accentHi.withAlpha (0.6f), -100, -10);
        const float x = freqToX (f, r);
        g.setColour (col::pink.withAlpha (0.25f));
        g.fillRect (Rectangle<float> (x, r.getY(), r.getRight() - x, r.getHeight() - 14));
        g.setColour (col::pink);
        g.drawLine (x, r.getY(), x, r.getBottom() - 14, 2.0f);
        g.setFont (mono (11.0f, true));
        g.setColour (col::text);
        g.drawText ("DE-ESSING  -" + juce::String (std::abs (proc.dsGrDb.load()), 1) + " dB", r.withTrimmedBottom (16).removeFromBottom (16), juce::Justification::centredLeft);
    }
    void setFreq (float x) { auto* p = proc.apvts.getParameter ("dsFreq"); p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (2000.0f, 12000.0f, xToFreq (x, plotArea)))); }
    void mouseDown (const juce::MouseEvent& e) override { proc.apvts.getParameter ("dsFreq")->beginChangeGesture(); dragging = true; setFreq (e.position.x); }
    void mouseDrag (const juce::MouseEvent& e) override { setFreq (e.position.x); repaint(); }
    void mouseUp (const juce::MouseEvent&) override { if (dragging) proc.apvts.getParameter ("dsFreq")->endChangeGesture(); dragging = false; }
};

class DriveScreen : public Screen
{
public:
    using Screen::Screen;
    float level = 0;
    void tick() override { level += (glaze::dbToGain (proc.outPeakDb.load()) - level) * 0.3f; }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const float amt = param ("drvAmount") * 0.01f;
        caption (g, r, "TRANSFER CURVE", "DRIVE " + juce::String (juce::roundToInt (amt * 100)) + " %");
        r.removeFromTop (20);
        auto sq = r.withSizeKeepingCentre (std::min (r.getWidth(), r.getHeight()), std::min (r.getWidth(), r.getHeight()));
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (sq.getX(), sq.getBottom(), sq.getRight(), sq.getY(), 1.0f);
        g.drawHorizontalLine ((int) sq.getCentreY(), sq.getX(), sq.getRight());
        g.drawVerticalLine ((int) sq.getCentreX(), sq.getY(), sq.getBottom());
        const float gain = 1.0f + amt * 15.0f;
        const float norm = std::max (std::abs (glaze::Drive::shape (1.0f, gain)), std::abs (glaze::Drive::shape (-1.0f, gain)));
        juce::Path p;
        for (int i = 0; i <= 200; ++i)
        {
            const float x = -1.0f + 2.0f * i / 200.0f;
            const float y = glaze::Drive::shape (x, gain) / norm;
            const float px = juce::jmap (x, -1.0f, 1.0f, sq.getX(), sq.getRight());
            const float py = juce::jmap (y, -1.0f, 1.0f, sq.getBottom(), sq.getY());
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        g.setColour (col::pink.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (7.0f));
        g.setColour (col::accentHi); g.strokePath (p, juce::PathStrokeType (2.4f));
        const float lv = juce::jlimit (0.0f, 1.0f, level);
        const float py = juce::jmap (glaze::Drive::shape (lv, gain) / norm, -1.0f, 1.0f, sq.getBottom(), sq.getY());
        const float px = juce::jmap (lv, -1.0f, 1.0f, sq.getX(), sq.getRight());
        g.setColour (juce::Colours::white); g.fillEllipse (px - 5, py - 5, 10, 10);
    }
};

class StereoScreen : public Screen
{
public:
    StereoScreen (GlazeProcessor& p, juce::String t) : Screen (p), title (std::move (t)) {}
    juce::String title;
    std::vector<float> l = std::vector<float> (1024), rr = std::vector<float> (1024);
    float corr = 1;
    void tick() override
    {
        proc.outScope.copyLast (1024, l.data(), rr.data());
        double s = 0, a = 0, b = 0;
        for (int i = 0; i < 1024; ++i) { s += l[(size_t) i] * rr[(size_t) i]; a += l[(size_t) i] * l[(size_t) i]; b += rr[(size_t) i] * rr[(size_t) i]; }
        const float c = a * b > 1e-12 ? (float) (s / std::sqrt (a * b)) : 1.0f;
        corr += (c - corr) * 0.2f;
    }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        caption (g, r, "STEREO FIELD", title);
        r.removeFromTop (16);
        auto bottom = r.removeFromBottom (16);
        const float rad = std::min (r.getWidth(), r.getHeight()) * 0.5f - 2;
        const auto c = r.getCentre();
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 1.0f);
        g.drawEllipse (c.x - rad * 0.5f, c.y - rad * 0.5f, rad, rad, 1.0f);
        g.drawLine (c.x - rad * 0.7f, c.y - rad * 0.7f, c.x + rad * 0.7f, c.y + rad * 0.7f);
        g.drawLine (c.x + rad * 0.7f, c.y - rad * 0.7f, c.x - rad * 0.7f, c.y + rad * 0.7f);
        g.setFont (mono (9.0f, true)); g.setColour (col::dim);
        g.drawText ("L", (int) (c.x - rad * 0.78f) - 6, (int) (c.y - rad * 0.78f) - 6, 12, 12, juce::Justification::centred);
        g.drawText ("R", (int) (c.x + rad * 0.78f) - 6, (int) (c.y - rad * 0.78f) - 6, 12, 12, juce::Justification::centred);
        float pk = 1e-3f; for (size_t i = 0; i < l.size(); ++i) pk = std::max (pk, std::max (std::abs (l[i]), std::abs (rr[i])));
        const float k = rad * 0.9f / std::max (pk, 0.25f);
        for (size_t i = 0; i < l.size(); i += 2)
        {
            const float side = (l[i] - rr[i]) * 0.7071f, mid = (l[i] + rr[i]) * 0.7071f;
            const float x = c.x + side * k, y = c.y - mid * k;
            g.setColour (col::pink.interpolatedWith (col::accentHi, (float) i / l.size()).withAlpha (0.55f));
            g.fillRect (x, y, 1.8f, 1.8f);
        }
        g.setFont (mono (10.5f, true)); g.setColour (col::text);
        g.drawText ("CORRELATION " + juce::String (corr, 2), bottom, juce::Justification::centredLeft);
        // correlation bar
        auto bar = bottom.removeFromRight (bottom.getWidth() * 0.4f).reduced (0, 5);
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (bar, 3);
        const float cx = juce::jmap (corr, -1.0f, 1.0f, bar.getX(), bar.getRight());
        g.setColour (col::pink); g.fillRoundedRectangle (cx - 3, bar.getY(), 6, bar.getHeight(), 2);
    }
};

class DelayScreen : public Screen
{
public:
    using Screen::Screen;
    void tick() override {}
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const bool sync = param ("dlSync") > 0.5f;
        const float bpm = proc.bpm.load();
        const float ms = sync ? glaze::divBeats[(int) param ("dlDiv")] * 60000.0f / juce::jlimit (30.0f, 300.0f, bpm) : param ("dlTime");
        caption (g, r, sync ? glaze::divNames[(int) param ("dlDiv")] + "  |  " + juce::String (juce::roundToInt (bpm)) + " BPM" : juce::String (juce::roundToInt (ms)) + " ms",
                 juce::String (juce::roundToInt (ms)) + " ms");
        r.removeFromTop (20);
        const bool ping = param ("dlPingPong") > 0.5f;
        const float fb = param ("dlFeedback") * 0.01f, mix = std::min (1.0f, param ("dlMix") * 0.01f * param ("space") * 0.01f);
        auto laneL = r.removeFromTop (r.getHeight() * 0.5f).reduced (0, 6), laneR = r.reduced (0, 6);
        g.setFont (mono (10.0f, true));
        for (auto [lane, name] : { std::pair { laneL, "LEFT" }, std::pair { laneR, "RIGHT" } })
        {
            g.setColour (juce::Colours::black.withAlpha (0.2f)); g.fillRoundedRectangle (lane, 6);
            g.setColour (col::dim); g.drawText (name, lane.reduced (6, 2), juce::Justification::topLeft);
        }
        const float total = ms * 9.0f;
        const float t0 = laneL.getX() + 40;
        const float w = laneL.getRight() - t0 - 6;
        // dry hit
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillRect (t0, laneL.getY() + 6, 3.0f, laneL.getHeight() - 12);
        g.fillRect (t0, laneR.getY() + 6, 3.0f, laneR.getHeight() - 12);
        for (int n = 1; n <= 8; ++n)
        {
            const float amp = (mix > 0.001f ? 0.3f + 0.7f * std::sqrt (mix) : 0.0f) * std::pow (fb, (float) (n - 1));
            const float x = t0 + w * (n * ms) / total;
            auto draw = [&] (Rectangle<float> lane, float a)
            {
                const float hh = juce::jlimit (2.0f, lane.getHeight() - 8, a * (lane.getHeight() - 8));
                g.setColour (col::pink.withAlpha (0.35f)); g.fillRect (x - 3, lane.getCentreY() - hh * 0.5f, 7.0f, hh);
                g.setColour (col::accentHi); g.fillRect (x - 1, lane.getCentreY() - hh * 0.5f, 3.0f, hh);
            };
            if (ping) draw ((n % 2) ? laneL : laneR, amp);
            else { draw (laneL, amp); draw (laneR, amp); }
        }
    }
};

class VerbScreen : public Screen
{
public:
    using Screen::Screen;
    History lvl { 160, 0.0f };
    void tick() override { lvl.push (glaze::dbToGain (proc.outPeakDb.load())); }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const float size = param ("rvSize") * 0.01f, pre = param ("rvPreDelay");
        const float rt = 0.35f + std::pow (size, 2.0f) * 5.5f;
        caption (g, r, "PRE-DELAY " + juce::String (juce::roundToInt (pre)) + " ms", "DECAY " + juce::String (rt, 2) + " s");
        r.removeFromTop (20);
        auto axis = r.removeFromBottom (14);
        const float maxT = std::max (1.5f, std::ceil ((rt + pre * 0.001f) * 1.25f));
        auto xOf = [&] (float t) { return r.getX() + r.getWidth() * t / maxT; };
        const float mix = std::min (1.5f, param ("rvMix") * 0.01f * param ("space") * 0.01f);
        const float peakH = 0.5f + 0.45f * std::min (1.0f, mix * 2.5f);
        juce::Path p; p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (xOf (pre * 0.001f), r.getBottom());
        for (float t = pre * 0.001f; t <= maxT; t += 0.02f)
        {
            const float env = std::pow (10.0f, -3.0f * (t - pre * 0.001f) / rt);
            p.lineTo (xOf (t), r.getBottom() - env * r.getHeight() * peakH);
        }
        p.lineTo (r.getRight(), r.getBottom()); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col::pink.withAlpha (0.85f), r.getX(), r.getY(), col::accent.withAlpha (0.1f), r.getX() + r.getWidth() * 0.6f, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (col::accentHi); g.strokePath (p, juce::PathStrokeType (1.5f));
        g.setFont (mono (9.0f)); g.setColour (col::accentHi.withAlpha (0.6f));
        for (int s = 0; s <= (int) maxT; ++s) g.drawText (juce::String (s) + "s", (int) xOf ((float) s) - 10, (int) axis.getY(), 20, 12, juce::Justification::centred);
    }
};

class LimitScreen : public Screen
{
public:
    using Screen::Screen;
    History lvl { 200, -100.0f }, gr { 200, 0.0f };
    void tick() override { lvl.push (proc.outPeakDb.load()); gr.push (proc.limGrDb.load()); }
    void drawContent (juce::Graphics& g, Rectangle<float> r) override
    {
        const float ceil = param ("limCeiling");
        caption (g, r, "LIMITING  -" + juce::String (std::abs (gr[gr.size() - 1]), 1) + " dB", "CEILING " + juce::String (ceil, 1) + " dB");
        r.removeFromTop (20);
        auto yOf = [&] (float db) { return juce::jmap (juce::jlimit (-40.0f, 3.0f, db), -40.0f, 3.0f, r.getBottom(), r.getY()); };
        const float dx = r.getWidth() / (lvl.size() - 1);
        for (int i = 0; i < lvl.size(); ++i)
        {
            const float y = yOf (lvl[i]);
            g.setColour (col::accentHi.withAlpha (0.25f + 0.6f * (float) i / lvl.size()));
            g.fillRect (r.getX() + i * dx, y, std::max (1.0f, dx - 0.5f), r.getBottom() - y);
            const float hgt = juce::jlimit (0.0f, 1.0f, gr[i] / 12.0f) * r.getHeight() * 0.5f;
            g.setColour (col::pink.withAlpha (0.8f));
            g.fillRect (r.getX() + i * dx, r.getY(), std::max (1.0f, dx - 0.5f), hgt);
        }
        g.setColour (juce::Colours::white);
        g.drawHorizontalLine ((int) yOf (ceil), r.getX(), r.getRight());
    }
};
} // namespace ui

// ============================================================================
// Page
Page::Page (GlazeProcessor& p, std::unique_ptr<Screen> s, const char* powerId, const juce::String& t, const juce::String& sub,
            std::vector<KnobSpec> right, std::vector<KnobSpec> bottom)
    : screen (std::move (s)), title (t), subtitle (sub)
{
    addAndMakeVisible (*screen);
    if (powerId != nullptr) { power = std::make_unique<PowerButton> (p.apvts, powerId); addAndMakeVisible (*power); }
    for (auto& k : right)  { rightKnobs.push_back (std::make_unique<Knob> (p.apvts, k.id, k.label)); addAndMakeVisible (*rightKnobs.back()); }
    for (auto& k : bottom) { bottomKnobs.push_back (std::make_unique<Knob> (p.apvts, k.id, k.label)); addAndMakeVisible (*bottomKnobs.back()); }
}

void Page::paint (juce::Graphics& g)
{
    auto side = getLocalBounds().toFloat().withTrimmedLeft (636).withHeight (36).withTrimmedRight (12);
    g.setFont (Fonts::get().logoFont (24.0f));
    g.setColour (col::text);
    g.drawText (title, side.removeFromTop (24), juce::Justification::centredLeft);
    g.setFont (mono (9.5f));
    g.setColour (col::dim);
    g.drawText (subtitle, side, juce::Justification::centredLeft);
    // divider above the bottom row
    g.setColour (col::edge.withAlpha (0.6f));
    g.drawHorizontalLine (348, 24.0f, (float) getWidth() - 24.0f);
}

void Page::resized()
{
    auto b = getLocalBounds();
    screen->setBounds (20, 0, 600, 336);
    if (power) power->setBounds (getWidth() - 56, 2, 32, 28);

    // right column: 2 x 2 knobs
    auto col = Rectangle<int> (636, 44, getWidth() - 636 - 16, 300);
    const int kw = col.getWidth() / 2, kh = 104;
    for (size_t i = 0; i < rightKnobs.size(); ++i)
        rightKnobs[i]->setBounds (col.getX() + (int) (i % 2) * kw, col.getY() + (int) (i / 2) * kh, kw, kh);
    int y = col.getY() + (int) ((rightKnobs.size() + 1) / 2) * kh + 6;

    if (homeLayout)
    {
        // extras (key + scale) overlay the screen
        int x = screen->getX() + 300 - 115;
        for (auto& [c, h] : extras) { c->setBounds (x, screen->getY() + 112, c == extras.front().first ? 70 : 150, h); x += 80; }
    }
    else
        for (auto& [c, h] : extras) { c->setBounds (col.getX() + 14, y, col.getWidth() - 28, h); y += h + 8; }

    // bottom row
    if (! bottomKnobs.empty())
    {
        auto row = Rectangle<int> (20, 356, b.getWidth() - 40, 100);
        const int w = std::min (130, row.getWidth() / (int) bottomKnobs.size());
        const int total = w * (int) bottomKnobs.size();
        int x = row.getX() + (row.getWidth() - total) / 2;
        for (auto& k : bottomKnobs) { k->setBounds (x, row.getY(), w, row.getHeight()); x += w; }
    }
}

// ============================================================================
// Tabs
std::vector<Rectangle<float>> TabBar::bounds() const
{
    std::vector<Rectangle<float>> out;
    const auto f = mono (10.5f, true);
    float total = 0;
    std::vector<float> widths;
    for (auto& t : tabs) { const float w = juce::GlyphArrangement::getStringWidth (f, t.name) + 14.0f; widths.push_back (w); total += w; }
    float x = std::max (0.0f, (getWidth() - total) * 0.5f);
    for (auto w : widths) { out.push_back ({ x, 0, w, (float) getHeight() }); x += w; }
    return out;
}
int TabBar::indexAt (juce::Point<int> p) const
{
    auto b = bounds();
    for (size_t i = 0; i < b.size(); ++i) if (b[i].contains (p.toFloat())) return (int) i;
    return -1;
}
void TabBar::paint (juce::Graphics& g)
{
    auto b = bounds();
    g.setFont (mono (10.5f, true));
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        const bool on = tabs[i].powerId == nullptr || proc.apvts.getRawParameterValue (tabs[i].powerId)->load() > 0.5f;
        const bool sel = (int) i == current;
        auto r = b[i];
        Colour c = sel ? juce::Colours::white : ((int) i == hover ? col::accentHi : col::text.withAlpha (0.8f));
        if (! on) c = c.withAlpha (0.3f);
        g.setColour (c);
        g.drawText (tabs[i].name, r, juce::Justification::centred);
        if (sel)
        {
            auto u = r.reduced (6, 0).removeFromBottom (3).translated (0, -6);
            g.setColour (col::pink.withAlpha (0.35f)); g.fillRoundedRectangle (u.expanded (0, 2), 2);
            g.setColour (col::pink); g.fillRoundedRectangle (u, 1.5f);
        }
    }
}
void TabBar::mouseDown (const juce::MouseEvent& e) { const int i = indexAt (e.getPosition()); if (i >= 0 && onSelect) onSelect (i); }
void TabBar::mouseMove (const juce::MouseEvent& e) { const int i = indexAt (e.getPosition()); if (i != hover) { hover = i; repaint(); } }

// ============================================================================
// Editor
GlazeEditor::GlazeEditor (GlazeProcessor& p) : AudioProcessorEditor (p), proc (p), tabs (p)
{
    setLookAndFeel (&lnf);
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);

    auto make = [&] (std::unique_ptr<Screen> s, const char* pid, const char* title, const char* sub, std::vector<KnobSpec> r, std::vector<KnobSpec> b)
    {
        pages.push_back (std::make_unique<Page> (proc, std::move (s), pid, title, sub, std::move (r), std::move (b)));
        return pages.back().get();
    };

    auto* home = make (std::make_unique<HomeScreen> (proc), nullptr, "home", "the whole chain, four knobs",
                       { { "tuneAmount", "Tune" }, { "tuneSpeed", "Speed" }, { "space", "Space" }, { "eqAir", "Air" } },
                       { { "inGain", "Input" }, { "punch", "Punch" }, { "width", "Width" }, { "mix", "Mix" }, { "outGain", "Output" } });
    home->homeLayout = true;
    home->addExtra (std::make_unique<ParamCombo> (proc.apvts, "key"), 24);
    home->addExtra (std::make_unique<ParamCombo> (proc.apvts, "scale"), 24);

    auto* tune = make (std::make_unique<TuneScreen> (proc), "tuneOn", "tune", "formant-safe pitch correction",
                       { { "tuneSpeed", "Retune" }, { "tuneAmount", "Amount" } },
                       { { "humanize", "Humanize" }, { "formant", "Formant" }, { "transpose", "Transpose" } });
    tune->addExtra (std::make_unique<ParamCombo> (proc.apvts, "key"), 26);
    tune->addExtra (std::make_unique<ParamCombo> (proc.apvts, "scale"), 26);

    make (std::make_unique<GateScreen> (proc), "gateOn", "gate", "kill the room, keep the breath",
          { { "gateThresh", "Threshold" }, { "gateRange", "Range" } }, { { "gateAttack", "Attack" }, { "gateRelease", "Release" } });
    make (std::make_unique<EqScreen> (proc), "eqOn", "eq", "shape the tone",
          { { "eqLowCut", "Low Cut" }, { "eqBody", "Body" }, { "eqPres", "Presence" }, { "eqAir", "Air" } },
          { { "eqBodyFreq", "Body Freq" }, { "eqPresFreq", "Pres Freq" } });
    make (std::make_unique<CompScreen> (proc), "compOn", "comp", "glue it to the front",
          { { "compThresh", "Threshold" }, { "compRatio", "Ratio" }, { "compAttack", "Attack" }, { "compRelease", "Release" } },
          { { "compMakeup", "Makeup" }, { "compMix", "Mix" }, { "punch", "Punch" } });
    make (std::make_unique<DeEssScreen> (proc), "dsOn", "de-ess", "tame the s's",
          { { "dsThresh", "Threshold" }, { "dsFreq", "Frequency" } }, { { "dsRange", "Range" } });
    make (std::make_unique<DriveScreen> (proc), "drvOn", "drive", "warm tape-ish grit",
          { { "drvAmount", "Drive" }, { "drvTone", "Tone" } }, { { "drvMix", "Mix" } });
    make (std::make_unique<StereoScreen> (proc, "DOUBLE"), "dblOn", "double", "instant double track",
          { { "dblMix", "Mix" }, { "dblDetune", "Detune" } }, { { "dblDelay", "Delay" }, { "width", "Width" } });
    make (std::make_unique<StereoScreen> (proc, "CHORUS"), "chOn", "chorus", "shimmer and movement",
          { { "chMix", "Mix" }, { "chDepth", "Depth" } }, { { "chRate", "Rate" } });
    auto* dly = make (std::make_unique<DelayScreen> (proc), "dlOn", "delay", "echoes in time",
                      { { "dlFeedback", "Feedback" }, { "dlMix", "Mix" } },
                      { { "dlTime", "Time (free)" }, { "dlLowCut", "Low Cut" }, { "dlHighCut", "High Cut" } });
    dly->addExtra (std::make_unique<ParamCombo> (proc.apvts, "dlDiv"), 26);
    dly->addExtra (std::make_unique<PillToggle> (proc.apvts, "dlSync", "SYNC TO TEMPO", "FREE TIME"), 24);
    dly->addExtra (std::make_unique<PillToggle> (proc.apvts, "dlPingPong", "PING-PONG", "STRAIGHT"), 24);
    make (std::make_unique<VerbScreen> (proc), "rvOn", "verb", "the space around you",
          { { "rvSize", "Size" }, { "rvMix", "Mix" } },
          { { "rvPreDelay", "Pre-Delay" }, { "rvDamp", "Damp" }, { "rvLowCut", "Low Cut" }, { "rvWidth", "Width" } });
    make (std::make_unique<LimitScreen> (proc), "limOn", "limit", "loud, never clipping",
          { { "limGain", "Gain" }, { "limCeiling", "Ceiling" } }, { { "limRelease", "Release" } });

    tabs.tabs = { { "HOME", nullptr }, { "TUNE", "tuneOn" }, { "GATE", "gateOn" }, { "EQ", "eqOn" }, { "COMP", "compOn" },
                  { "DE-ESS", "dsOn" }, { "DRIVE", "drvOn" }, { "DOUBLE", "dblOn" }, { "CHORUS", "chOn" }, { "DELAY", "dlOn" },
                  { "VERB", "rvOn" }, { "LIMIT", "limOn" } };
    tabs.onSelect = [this] (int i) { showPage (i); };
    addAndMakeVisible (tabs);

    for (auto& pg : pages) addChildComponent (*pg);

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.onChange = [this] { const int i = presetBox.getSelectedId() - 1; if (i >= 0 && i != proc.getCurrentProgram()) proc.setCurrentProgram (i); };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); syncPresetBox(); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); syncPresetBox(); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    lowLatency = std::make_unique<PillToggle> (proc.apvts, "lowLatency", "LOW LATENCY ON", "LOW LATENCY OFF");
    lowLatency->setTooltip ("Shorter tuning look-ahead for tracking live. Slightly less accurate on very low notes.");
    addAndMakeVisible (*lowLatency);

    syncPresetBox();
    setSize (940, 560);
    showPage (0);
    startTimerHz (30);
}

GlazeEditor::~GlazeEditor()
{
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void GlazeEditor::showPage (int i)
{
    current = i; tabs.current = i;
    for (size_t k = 0; k < pages.size(); ++k) pages[k]->setVisible ((int) k == i);
    tabs.repaint();
}

void GlazeEditor::syncPresetBox() { presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification); }

void GlazeEditor::timerCallback()
{
    auto& pg = *pages[(size_t) current];
    pg.screen->tick();
    pg.screen->repaint();
    static int n = 0;
    if (++n % 6 == 0) { tabs.repaint(); syncPresetBox(); }
}

void GlazeEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.fillAll (col::bg);
    auto shell = r.reduced (6.0f);
    juce::ColourGradient grad (col::shellHi, shell.getX(), shell.getY(), col::shell, shell.getX(), shell.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (shell, 18.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (shell, 18.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (shell.reduced (2.0f), 16.0f, 1.0f);

    // header logo
    auto path = ui::logoPath (30.0f);
    path.applyTransform (juce::AffineTransform::shear (-0.2f, 0));
    auto pb = path.getBounds();
    path.applyTransform (juce::AffineTransform::translation (26 - pb.getX(), 30 - pb.getCentreY()));
    juce::DropShadow (col::pink.withAlpha (0.6f), 10, {}).drawForPath (g, path);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0, 16, col::pink, 0, 44, false));
    g.fillPath (path);

    // header divider
    g.setColour (col::edge.withAlpha (0.7f));
    g.drawHorizontalLine (56, 18.0f, (float) getWidth() - 18.0f);
}

void GlazeEditor::resized()
{
    tabs.setBounds (100, 14, 560, 32);
    prevBtn.setBounds (getWidth() - 236, 17, 26, 26);
    presetBox.setBounds (getWidth() - 206, 17, 150, 26);
    nextBtn.setBounds (getWidth() - 52, 17, 26, 26);
    for (auto& pg : pages) pg->setBounds (0, 66, getWidth(), 460);
    lowLatency->setBounds (getWidth() / 2 - 80, getHeight() - 36, 160, 22);
}

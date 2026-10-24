#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace lui
{
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Typeface::Ptr tfLogo, tfMono;
};

// One effect column: on/off dot + name, knob, value
class Card : public juce::Component
{
public:
    Card (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& onId, const juce::String& name);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Slider slider;
private:
    juce::AudioProcessorValueTreeState& s;
    juce::String onId, name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// The big BUILD dial with the rocket inside it.
class RocketDial : public juce::Component
{
public:
    explicit RocketDial (LiftoffProcessor& p) : proc (p) {}
    void tick (float dt);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String status() const;
    bool blasting() const { return blastAge >= 0.0f; }
private:
    void drawRocket (juce::Graphics&, juce::Point<float> base, float k, float b, float flameLen);
    void drawFace (juce::Graphics&, juce::Point<float> c, float r, float b);
    LiftoffProcessor& proc;
    float time = 0, b = 0, hover = 0, blastAge = -1, blastY = 0, blastV = 0, landing = 0;
    float recentMax = 0, startDragValue = 0;
    struct Puff { juce::Point<float> pos, vel; float r, life; };
    std::vector<Puff> smoke;
    juce::Random rng;
};

// Filter response + wet amounts
class Graph : public juce::Component
{
public:
    explicit Graph (LiftoffProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    LiftoffProcessor& proc;
};

// Auto build row: on/off, drop bar, length, live status
class AutoRow : public juce::Component
{
public:
    explicit AutoRow (LiftoffProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void setBar (int);
    int bar() const;
    LiftoffProcessor& proc;
    juce::ComboBox len;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lenAtt;
    juce::Rectangle<float> togR, minusR, numR, plusR, statusR;
    int dragStartBar = 0; bool draggingNum = false;
};

// Hold to build, let go to drop
class HoldButton : public juce::Component
{
public:
    explicit HoldButton (LiftoffProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick (float dt);
    bool held = false;
private:
    LiftoffProcessor& proc;
};
}

class LiftoffEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LiftoffEditor (LiftoffProcessor&);
    ~LiftoffEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    lui::RocketDial& getDial() { return dial; }
private:
    LiftoffProcessor& proc;
    lui::LnF lnf;
    lui::RocketDial dial;
    lui::Graph graph;
    lui::HoldButton hold;
    lui::AutoRow autoRow;
    juce::ComboBox delayTime;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayAtt;
    std::vector<std::unique_ptr<lui::Card>> cards;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiftoffEditor)
};

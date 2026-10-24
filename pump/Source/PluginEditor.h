#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace pumpui
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
    juce::Typeface::Ptr logo, mono;
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& label);
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (16)); }
    void paint (juce::Graphics&) override;
    juce::Slider slider;
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Display : public juce::Component
{
public:
    explicit Display (PumpProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    float scLevel = -100;
private:
    juce::Rectangle<float> plot() const;
    juce::Point<float> handlePos (int) const;
    PumpProcessor& proc;
    int active = -1;
};

class ModeSwitch : public juce::Component, private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit ModeSwitch (juce::AudioProcessorValueTreeState&);
    ~ModeSwitch() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void parameterChanged (const juce::String&, float) override { juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<ModeSwitch> (this)] { if (sp) sp->repaint(); }); }
    juce::AudioProcessorValueTreeState& s;
};
}

class PumpEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PumpEditor (PumpProcessor&);
    ~PumpEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    PumpProcessor& proc;
    pumpui::LnF lnf;
    pumpui::Display display;
    pumpui::ModeSwitch mode;
    juce::ComboBox rate;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAtt;
    std::vector<std::unique_ptr<pumpui::Knob>> knobs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PumpEditor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"

CassettepreampAudioProcessorEditor::CassettepreampAudioProcessorEditor (CassettepreampAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 220);

    // Setup each knob and label
    auto setupKnob = [&](juce::Slider& knob, juce::Label& label, const juce::String& text)
    {
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        addAndMakeVisible(knob);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(label);
    };

    setupKnob(driveKnob,  driveLabel,  "DRIVE");
    setupKnob(toneKnob,   toneLabel,   "TONE");
    setupKnob(wowKnob,    wowLabel,    "WOW & FLUTTER");
    setupKnob(noiseKnob,  noiseLabel,  "NOISE");
    setupKnob(outputKnob, outputLabel, "OUTPUT");

    // Connect knobs to parameters
    driveAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "drive",  driveKnob);
    toneAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "tone",   toneKnob);
    wowAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "wow",    wowKnob);
    noiseAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "noise",  noiseKnob);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "output", outputKnob);
}

CassettepreampAudioProcessorEditor::~CassettepreampAudioProcessorEditor() {}

void CassettepreampAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background — dark brownish cassette colour
    g.fillAll(juce::Colour(0xff2a1f1a));

    // Title
    g.setColour(juce::Colour(0xffd4a96a));
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawFittedText("CASSETTE PREAMP", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void CassettepreampAudioProcessorEditor::resized()
{
    int knobSize = 80;
    int labelHeight = 20;
    int startX = 20;
    int knobY = 50;
    int spacing = (getWidth() - startX * 2) / 5;

    auto placeKnob = [&](juce::Slider& knob, juce::Label& label, int index)
    {
        int x = startX + index * spacing;
        knob.setBounds(x, knobY, knobSize, knobSize);
        label.setBounds(x, knobY + knobSize + 2, knobSize, labelHeight);
    };

    placeKnob(driveKnob,  driveLabel,  0);
    placeKnob(toneKnob,   toneLabel,   1);
    placeKnob(wowKnob,    wowLabel,    2);
    placeKnob(noiseKnob,  noiseLabel,  3);
    placeKnob(outputKnob, outputLabel, 4);
}

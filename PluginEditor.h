#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CassettepreampAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CassettepreampAudioProcessorEditor (CassettepreampAudioProcessor&);
    ~CassettepreampAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CassettepreampAudioProcessor& audioProcessor;

    // Sliders
    juce::Slider driveKnob, toneKnob, wowKnob, noiseKnob, outputKnob;

    // Labels
    juce::Label driveLabel, toneLabel, wowLabel, noiseLabel, outputLabel;

    // Attachments (connect sliders to parameters)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wowAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CassettepreampAudioProcessorEditor)
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DrivePedalAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DrivePedalAudioProcessorEditor (DrivePedalAudioProcessor&);
    ~DrivePedalAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider driveKnob, levelKnob, toneKnob;
    juce::ToggleButton bypass { "Bypass" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveKnobAttachment, levelKnobAttachment, toneKnobAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrivePedalAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrivePedalAudioProcessorEditor)
};

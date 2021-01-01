/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrivePedalAudioProcessorEditor::DrivePedalAudioProcessorEditor (DrivePedalAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    driveKnob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 25);
    addAndMakeVisible(driveKnob);
    
    levelKnob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    levelKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 25);
    addAndMakeVisible(levelKnob);
    
    toneKnob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    toneKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 25);
    addAndMakeVisible(toneKnob);

    addAndMakeVisible(bypass);
    
    driveKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.state, "DRIVE", driveKnob);
    levelKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.state, "LEVEL", levelKnob);
    toneKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.state, "TONE", toneKnob);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.state, "BYPASS", bypass);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(250, 550);
}

DrivePedalAudioProcessorEditor::~DrivePedalAudioProcessorEditor()
{
}

//==============================================================================
void DrivePedalAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::seagreen);

    g.setColour(juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("MINICREAMER", getLocalBounds(), juce::Justification::centredBottom, 1);
}

void DrivePedalAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    toneKnob.setBounds(10, 10, 120, 120);
    levelKnob.setBounds(getWidth() - 130, 10, 120, 120);

    driveKnob.setBounds(getWidth() / 2 - 100, getHeight() / 2 - 100, 200, 200);
    
    bypass.setBounds(getWidth() / 2 - 40, getHeight() - 120, 80, 25);
}

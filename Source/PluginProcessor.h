/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define SIGNAL_HP_FREQ 720.484f;
#define DRIVE_MULTIPLIER 1.06383f;
#define DRIVE_OFFSET 11.851f;
#define HARD_CLIP_FACTOR 1.4f;
#define OUTPUT_FACTOR .5f;
#define OUTPUT_LP_FREQ 723.431f;

//==============================================================================
/**
*/
class DrivePedalAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DrivePedalAudioProcessor();
    ~DrivePedalAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
//    void update();
    
//    static float ocd(float sample);
    static float cubicPolynomial(float sample);
private:
    using Bias = juce::dsp::Bias<float>;
    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain = juce::dsp::Gain<float>;
    using OverSampling = juce::dsp::Oversampling<float>;
    using Shaper = juce::dsp::WaveShaper<float>;
    
    OverSampling ov { 2, 4, OverSampling::filterHalfBandPolyphaseIIR, true, false };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
//    juce::dsp::ProcessorChain<Gain, Bias, Shaper, FilterBand, Gain, FilterBand, FilterBand> drive;
//    juce::dsp::ProcessorChain<Gain, Shaper, Gain> drive;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrivePedalAudioProcessor)
};

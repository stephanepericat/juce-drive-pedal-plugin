/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define DEFAULT_Q .7071f
#define SIGNAL_HP_FREQ 320.484f
#define DRIVE_MULTIPLIER 1.06383f
#define DRIVE_OFFSET 11.851f
#define CLIP_LIMIT 1.4f
#define OUTPUT_FACTOR .5f
#define OUTPUT_LP_FREQ 2223.431f

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
    
    void updateProcessors();

    static float cubicPolynomial(float sample);
    static float hardClip(float sample, float minmax);
private:
    using Bias = juce::dsp::Bias<float>;
    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain = juce::dsp::Gain<float>;
    using Mixer = juce::dsp::DryWetMixer<float>;
    using OverSampling = juce::dsp::Oversampling<float>;
    using Shaper = juce::dsp::WaveShaper<float>;
    
    using DistortionProcessor = juce::dsp::ProcessorChain<FilterBand, Gain, Shaper, Shaper, Gain, FilterBand>;
    using PostProcessor = juce::dsp::ProcessorChain<FilterBand, Gain>;
    
    enum DistortionStages {
        hpFilterIndex,
        preGainIndex,
        softClipIndex,
        hardClipIndex,
        postGainIndex,
        lpFilterIndex,
    };
    
    enum PostStages {
        toneIndex,
        volumeIndex,
    };
    
    DistortionProcessor distortionProcessor;
    PostProcessor postProcessor;
    
    OverSampling ov { 2, 4, OverSampling::filterHalfBandPolyphaseIIR, true, false };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrivePedalAudioProcessor)
};

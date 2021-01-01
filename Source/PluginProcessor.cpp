/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrivePedalAudioProcessor::DrivePedalAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), state(*this, nullptr, "parameters", createParams())
#endif
{
}

DrivePedalAudioProcessor::~DrivePedalAudioProcessor()
{
}

//==============================================================================
const juce::String DrivePedalAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrivePedalAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DrivePedalAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DrivePedalAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DrivePedalAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrivePedalAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DrivePedalAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrivePedalAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DrivePedalAudioProcessor::getProgramName (int index)
{
    return {};
}

void DrivePedalAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DrivePedalAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    distortionProcessor.prepare(spec);
    postProcessor.prepare(spec);
    
    distortionProcessor.reset();
    postProcessor.reset();
}

void DrivePedalAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrivePedalAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DrivePedalAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // bypass ?
    auto rawBypassValue = state.getRawParameterValue("BYPASS");
    int bypassValue = rawBypassValue->load();
    
    if(bypassValue == 1)
        return;
    
    // store dry signal for later
    juce::AudioBuffer<float> drySignal((size_t)0, buffer.getNumSamples());
    drySignal = buffer;
    juce::dsp::AudioBlock<float> dryBlock = juce::dsp::AudioBlock<float>(drySignal);
    // create processing context for signal
    juce::dsp::AudioBlock<float> audioBlock = juce::dsp::AudioBlock<float>(buffer);
    juce::dsp::ProcessContextReplacing<float> context = juce::dsp::ProcessContextReplacing<float>(audioBlock);
    
    /// ---------------------------------------
    /// Processing
    /// ---------------------------------------
    
    /// 1. update processors
    updateProcessors();
    
    /// 2. oversample up
    ov.processSamplesUp(audioBlock);

    /// 3. distortion processor
    distortionProcessor.process(context);
    
    /// 4. merge with dry signal and tame volume
    auto& outputBlock = context.getOutputBlock();
    outputBlock.add(dryBlock);
    outputBlock.multiplyBy(OUTPUT_FACTOR);
    
    /// 5. post processor
    postProcessor.process(context);
    
    /// 6. oversample down
    ov.processSamplesDown(audioBlock);
}

//==============================================================================
bool DrivePedalAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DrivePedalAudioProcessor::createEditor()
{
    return new DrivePedalAudioProcessorEditor (*this);
}

//==============================================================================
void DrivePedalAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DrivePedalAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrivePedalAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout DrivePedalAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DRIVE", "Drive", juce::NormalisableRange<float>(0.f, 24.f, .01f), 12.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LEVEL", "Level", juce::NormalisableRange<float>(0.f, 2.f, .01f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TONE", "Tone", juce::NormalisableRange<float>(.1f, 1.5f, .01f), .8f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("BYPASS", "Bypass", 0, 1, 0));
    
    return { params.begin(), params.end() };
}

float DrivePedalAudioProcessor::cubicPolynomial(float sample)
{
    return ((3 / 2) * sample) - ((1 / 2) * std::pow(sample, 3));
}

float DrivePedalAudioProcessor::hardClip(float sample, float minmax)
{
    return juce::jlimit(-(minmax), minmax, sample);
}

void DrivePedalAudioProcessor::updateProcessors()
{
    float sampleRate = getSampleRate();

    auto rawDriveValue = state.getRawParameterValue("DRIVE");
    float driveValue = rawDriveValue->load();
    auto rawToneValue = state.getRawParameterValue("TONE");
    float toneValue = rawToneValue->load();
    auto rawLevelValue = state.getRawParameterValue("LEVEL");
    float levelValue = rawLevelValue->load();
    
    *distortionProcessor.get<hpFilterIndex>().state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(sampleRate, SIGNAL_HP_FREQ);
    distortionProcessor.get<preGainIndex>().setGainDecibels((DRIVE_MULTIPLIER * driveValue) + DRIVE_OFFSET);
    distortionProcessor.get<softClipIndex>().functionToUse = cubicPolynomial;
    distortionProcessor.get<hardClipIndex>().functionToUse = [](float sample) {
        return hardClip(sample, CLIP_LIMIT);
    };
    distortionProcessor.get<postGainIndex>().setGainLinear(OUTPUT_FACTOR);
    *distortionProcessor.get<lpFilterIndex>().state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, OUTPUT_LP_FREQ);
    
    *postProcessor.get<toneIndex>().state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, OUTPUT_LP_FREQ, DEFAULT_Q, toneValue);
    postProcessor.get<volumeIndex>().setGainLinear(levelValue);
}

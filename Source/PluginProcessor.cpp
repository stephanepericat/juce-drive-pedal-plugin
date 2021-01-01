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
    
    hp.prepare(spec);
    level.prepare(spec);
    lp.prepare(spec);
    mixer.prepare(spec);
    tone.prepare(spec);
    
    hp.reset();
    level.reset();
    lp.reset();
    mixer.reset();
    tone.reset();
    
//    ov.initProcessing(samplesPerBlock);
//    ov.reset();
    
//    drive.prepare(spec);
//    drive.reset();
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
    
    // store dry signal for later
    juce::AudioBuffer<float> drySignal((size_t)0, buffer.getNumSamples());
    drySignal = buffer;
    juce::dsp::AudioBlock<float> dryBlock = juce::dsp::AudioBlock<float>(drySignal);
    // create processing context for signal
    juce::dsp::AudioBlock<float> audioBlock = juce::dsp::AudioBlock<float>(buffer);
    juce::dsp::ProcessContextReplacing<float> context = juce::dsp::ProcessContextReplacing<float>(audioBlock);
    
    // ---------------------------------------
    // Main Process
    // ---------------------------------------
    
    // 1. high pass at SIGNAL_HP_FREQ
    *hp.state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(getSampleRate(), SIGNAL_HP_FREQ);
    hp.process(context);
    // 2. gain (DRIVE_MULTIPLIER * DRIVE + DRIVE_OFFSET)
    auto rawDriveValue = state.getRawParameterValue("DRIVE");
    float driveValue = rawDriveValue->load();
    drive.setGainDecibels((DRIVE_MULTIPLIER * driveValue) + DRIVE_OFFSET);
    drive.process(context);
    // 3. cubic waveshaping (soft clip)
    clip.functionToUse = cubicPolynomial;
    clip.process(context);
    // 4. hard clip DRIVE_OFFSET
    clamp.functionToUse = [](float sample) {
        return hardClip(sample, DRIVE_OFFSET);
    };
    clamp.process(context);
    // 5. trim gain OUTPUT_FACTOR
    trim.setGainLinear(OUTPUT_FACTOR);
    trim.process(context);
    // 6. low pass at OUTPUT_LP_FREQ
    *lp.state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(getSampleRate(), OUTPUT_LP_FREQ);
    lp.process(context);
    // 7. merge with dry signal and tame volume
    auto& outputBlock = context.getOutputBlock();
    outputBlock.add(dryBlock);
    outputBlock.multiplyBy(OUTPUT_FACTOR);
    
    // ---------------------------------------
    // Post processing
    // ---------------------------------------
    
    // 8. Tone
    auto rawToneValue = state.getRawParameterValue("TONE");
    float toneValue = rawToneValue->load();
    *tone.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), OUTPUT_LP_FREQ, DEFAULT_Q, toneValue);
    tone.process(context);
    // 9. Volume
    auto rawLevelValue = state.getRawParameterValue("LEVEL");
    float levelValue = rawLevelValue->load();
    level.setGainLinear(levelValue);
    level.process(context);
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
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DRIVE", "Drive", juce::NormalisableRange<float>(12.f, 36.f, .1f), 24.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LEVEL", "Level", juce::NormalisableRange<float>(0.f, 2.f, .01f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TONE", "Tone", juce::NormalisableRange<float>(.1f, 1.5f, .01f), .8f));
    
    return { params.begin(), params.end() };
}

//void DrivePedalAudioProcessor::update()
//{
//    float sampleRate = getSampleRate();
    
//    auto rawGainValue = state.getRawParameterValue("GAIN");
//    float gainValue = rawGainValue->load();
//    float postGainValue = -((2 / juce::MathConstants<float>::pi) * gainValue);

//    drive.get<0>().setGainDecibels(gainValue);
//    drive.get<1>().setBias(.4f);
//    drive.get<2>().functionToUse = juce::dsp::FastMathApproximations::tanh;
//    drive.get<2>().functionToUse = std::tanh;
//    drive.get<2>().functionToUse = ocd;
//    drive.get<1>().functionToUse = cubicPolynomial;
//    drive.get<2>().functionToUse = [](float sample)
//    {
//        return sample;
//    };

//    *drive.get<3>().state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 5.f, .7071f);
//    drive.get<2>().setGainDecibels(postGainValue);
//    *drive.get<5>().state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(sampleRate, 200.f);
//    *drive.get<6>().state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, 5000.f);
//}

/**
 * Taken from: https://github.com/JanosGit/Schrammel_OJD/blob/master/Source/Waveshaper.h
 */
//float DrivePedalAudioProcessor::ocd(float sample)
//{
//    float out = sample;
//
//    if (sample <= -1.7f)
//        out = -1.0f;
//    else if ((sample > -1.7f) && (sample < -0.3f))
//    {
//        sample += 0.3f;
//        out = sample + (sample * sample) / (4 * (1 - 0.3f)) - 0.3f;
//    }
//    else if ((sample > 0.9f) && (sample < 1.1f))
//    {
//        sample -= 0.9f;
//        out = sample - (sample * sample) / (4 * (1 - 0.9f)) + 0.9f;
//    }
//    else if (sample > 1.1f)
//        out = 1.0f;
//
//    return out;
//}

float DrivePedalAudioProcessor::cubicPolynomial(float sample)
{
    return ((3 / 2) * sample) - ((1 / 2) * std::pow(sample, 3));
}

float DrivePedalAudioProcessor::hardClip(float sample, float minmax)
{
    return juce::jlimit(-(minmax), minmax, sample);
}

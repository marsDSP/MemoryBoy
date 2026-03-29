#include "MemoryBoyProcessor.h"

namespace
{
    constexpr auto stateId = "PARAMETERS";
    constexpr auto delayMsParameterId = "delayMs";
    constexpr auto feedbackParameterId = "feedback";
    constexpr auto mixParameterId = "mix";
    constexpr auto modParameterId = "mod";
    constexpr auto diffusorParameterId = "diffusor";
    constexpr auto inputFilterParameterId = "inputFilterHz";
    constexpr auto outputFilterParameterId = "outputFilterHz";
}

//==============================================================================
MemoryBoyProcessor::MemoryBoyProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters (*this, nullptr, stateId, createParameterLayout())
{
    delayMsParameter = parameters.getRawParameterValue (delayMsParameterId);
    feedbackParameter = parameters.getRawParameterValue (feedbackParameterId);
    mixParameter = parameters.getRawParameterValue (mixParameterId);
    modParameter = parameters.getRawParameterValue (modParameterId);
    diffusorParameter = parameters.getRawParameterValue (diffusorParameterId);
    inputFilterParameter = parameters.getRawParameterValue (inputFilterParameterId);
    outputFilterParameter = parameters.getRawParameterValue (outputFilterParameterId);
}

MemoryBoyProcessor::~MemoryBoyProcessor()
{
}

//==============================================================================
const juce::String MemoryBoyProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MemoryBoyProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MemoryBoyProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MemoryBoyProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MemoryBoyProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

juce::AudioProcessorValueTreeState::ParameterLayout MemoryBoyProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { delayMsParameterId, 1 },
        "Delay",
        juce::NormalisableRange<float> { 1.0f, 2000.0f, 1.0f, 0.4f },
        350.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { feedbackParameterId, 1 },
        "Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.95f, 0.01f },
        0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { mixParameterId, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f },
        0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { modParameterId, 1 },
        "Mod",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f, 2.0f },
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { diffusorParameterId, 1 },
        "Diffusor",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f, 3.0f },
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inputFilterParameterId, 1 },
        "Input Filter",
        juce::NormalisableRange<float> { 200.0f, 20000.0f, 1.0f, 0.35f },
        MarsDSP::DSP::BBDFilterSpec::inputFilterOriginalCutoff,
        "Hz"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outputFilterParameterId, 1 },
        "Output Filter",
        juce::NormalisableRange<float> { 200.0f, 20000.0f, 1.0f, 0.35f },
        MarsDSP::DSP::BBDFilterSpec::outputFilterOriginalCutoff,
        "Hz"));

    return layout;
}

void MemoryBoyProcessor::updateProcessingParameters()
{
    const auto sampleRate = static_cast<float> (getSampleRate());

    if (sampleRate <= 0.0f)
        return;

    const auto delayMs = delayMsParameter != nullptr ? delayMsParameter->load() : 350.0f;
    const auto modAmount = modParameter != nullptr ? modParameter->load() * 0.01f : 0.0f;
    const auto diffusorAmount = diffusorParameter != nullptr ? diffusorParameter->load() * 0.01f : 0.0f;
    const auto inputFilterHz = inputFilterParameter != nullptr
                                   ? inputFilterParameter->load()
                                   : MarsDSP::DSP::BBDFilterSpec::inputFilterOriginalCutoff;
    const auto outputFilterHz = outputFilterParameter != nullptr
                                    ? outputFilterParameter->load()
                                    : MarsDSP::DSP::BBDFilterSpec::outputFilterOriginalCutoff;

    brigadeDelay.setDelay (delayMs * 0.001f * sampleRate);
    brigadeDelay.setTapModulation (juce::jlimit (0.0f, 1.0f, modAmount));
    brigadeDelay.setDiffusor (juce::jlimit (0.0f, 1.0f, diffusorAmount));
    brigadeDelay.setInputFilterFreq (inputFilterHz);
    brigadeDelay.setOutputFilterFreq (outputFilterHz);
}

void MemoryBoyProcessor::prepareProcessingState (double sampleRate, int samplesPerBlock, juce::uint32 numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (juce::jmax (samplesPerBlock, 1));
    spec.numChannels = juce::jmax<juce::uint32> (numChannels, 2u);

    brigadeDelay.prepare (spec);
    brigadeDelay.reset();

    feedbackSamples.assign (static_cast<size_t> (spec.numChannels), 0.0f);
    preparedBlockSize = static_cast<int> (spec.maximumBlockSize);
    preparedNumChannels = spec.numChannels;

    updateProcessingParameters();
}

int MemoryBoyProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MemoryBoyProcessor::getCurrentProgram()
{
    return 0;
}

void MemoryBoyProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String MemoryBoyProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void MemoryBoyProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void MemoryBoyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    prepareProcessingState (sampleRate,
                            samplesPerBlock,
                            static_cast<juce::uint32> (juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels())));
}

void MemoryBoyProcessor::releaseResources()
{
    feedbackSamples.clear();
    brigadeDelay.free();
    preparedBlockSize = 0;
    preparedNumChannels = 0;
}

bool MemoryBoyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #endif

    return true;
  #endif
}

void MemoryBoyProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto bufferNumChannels = buffer.getNumChannels();

    if (buffer.getNumSamples() == 0 || bufferNumChannels == 0 || totalNumInputChannels == 0)
        return;

    if (preparedNumChannels < static_cast<juce::uint32> (bufferNumChannels)
        || buffer.getNumSamples() > preparedBlockSize)
    {
        prepareProcessingState (getSampleRate(),
                                buffer.getNumSamples(),
                                static_cast<juce::uint32> (bufferNumChannels));
    }

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (feedbackSamples.size() != static_cast<size_t> (totalNumInputChannels))
        feedbackSamples.assign (static_cast<size_t> (totalNumInputChannels), 0.0f);

    updateProcessingParameters();
    brigadeDelay.prepareModulationBlock (buffer.getNumSamples());

    const auto feedback = jlimit (0.0f, 0.95f, feedbackParameter != nullptr ? feedbackParameter->load() : 0.35f);
    const auto mix = jlimit (0.0f, 1.0f, mixParameter != nullptr ? mixParameter->load() : 0.35f);

    std::vector<float*> channelData (static_cast<size_t> (totalNumInputChannels), nullptr);
    std::vector inputFrame (static_cast<size_t> (totalNumInputChannels), 0.0f);

    for (auto channel = 0; channel < totalNumInputChannels; ++channel)
        channelData[static_cast<size_t> (channel)] = buffer.getWritePointer (channel);

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (auto channel = 0; channel < totalNumInputChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t> (channel);
            const auto drySample = channelData[channelIndex][sample];
            inputFrame[channelIndex] = drySample + feedbackSamples[channelIndex] * feedback;
        }

        const auto& delayedFrame = brigadeDelay.processFrame (inputFrame.data(), totalNumInputChannels);

        for (auto channel = 0; channel < totalNumInputChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t> (channel);
            const auto drySample = channelData[channelIndex][sample];
            const auto delayedSample = delayedFrame[channelIndex];
            feedbackSamples[channelIndex] = delayedSample;
            channelData[channelIndex][sample] = drySample * (1.0f - mix) + delayedSample * mix;
        }
    }
}

//==============================================================================
bool MemoryBoyProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MemoryBoyProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void MemoryBoyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto stateXml = parameters.copyState().createXml())
        copyXmlToBinary (*stateXml, destData);
}

void MemoryBoyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto stateXml = getXmlFromBinary (data, sizeInBytes);
        stateXml != nullptr && stateXml->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*stateXml));
        updateProcessingParameters();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MemoryBoyProcessor();
}

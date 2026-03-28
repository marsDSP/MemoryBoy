#include "MemoryBoyProcessor.h"

namespace
{
    constexpr auto stateId = "PARAMETERS";
    constexpr auto delayMsParameterId = "delayMs";
    constexpr auto feedbackParameterId = "feedback";
    constexpr auto mixParameterId = "mix";
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
    const auto inputFilterHz = inputFilterParameter != nullptr
                                   ? inputFilterParameter->load()
                                   : MarsDSP::DSP::BBDFilterSpec::inputFilterOriginalCutoff;
    const auto outputFilterHz = outputFilterParameter != nullptr
                                    ? outputFilterParameter->load()
                                    : MarsDSP::DSP::BBDFilterSpec::outputFilterOriginalCutoff;

    brigadeDelay.setDelay (delayMs * 0.001f * sampleRate);
    brigadeDelay.setInputFilterFreq (inputFilterHz);
    brigadeDelay.setOutputFilterFreq (outputFilterHz);
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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()));

    brigadeDelay.prepare (spec);
    brigadeDelay.reset();

    feedbackSamples.assign (static_cast<size_t> (spec.numChannels), 0.0f);
    updateProcessingParameters();
}

void MemoryBoyProcessor::releaseResources()
{
    feedbackSamples.clear();
    brigadeDelay.free();
}

bool MemoryBoyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

void MemoryBoyProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (buffer.getNumSamples() == 0 || totalNumInputChannels == 0)
        return;

    if (feedbackSamples.size() != static_cast<size_t> (totalNumInputChannels))
        feedbackSamples.assign (static_cast<size_t> (totalNumInputChannels), 0.0f);

    updateProcessingParameters();

    const auto feedback = juce::jlimit (0.0f, 0.95f, feedbackParameter != nullptr ? feedbackParameter->load() : 0.35f);
    const auto mix = juce::jlimit (0.0f, 1.0f, mixParameter != nullptr ? mixParameter->load() : 0.35f);

    for (auto channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        auto delayedSample = feedbackSamples[static_cast<size_t> (channel)];

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto drySample = channelData[sample];

            brigadeDelay.pushSample (channel, drySample + delayedSample * feedback);
            delayedSample = brigadeDelay.popSample (channel);

            channelData[sample] = drySample * (1.0f - mix) + delayedSample * mix;
        }

        feedbackSamples[static_cast<size_t> (channel)] = delayedSample;
    }
}

//==============================================================================
bool MemoryBoyProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MemoryBoyProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
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

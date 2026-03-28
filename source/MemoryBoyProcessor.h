#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/brigade_wrapper.h"

//==============================================================================
class MemoryBoyProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    MemoryBoyProcessor();
    ~MemoryBoyProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void prepareProcessingState (double sampleRate, int samplesPerBlock, juce::uint32 numChannels);
    void updateProcessingParameters();

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    MarsDSP::DSP::BBDWrapper<4096> brigadeDelay;
    std::vector<float> feedbackSamples;
    int preparedBlockSize = 0;
    juce::uint32 preparedNumChannels = 0;

    std::atomic<float>* delayMsParameter = nullptr;
    std::atomic<float>* feedbackParameter = nullptr;
    std::atomic<float>* mixParameter = nullptr;
    std::atomic<float>* modParameter = nullptr;
    std::atomic<float>* diffusorParameter = nullptr;
    std::atomic<float>* inputFilterParameter = nullptr;
    std::atomic<float>* outputFilterParameter = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryBoyProcessor)
};

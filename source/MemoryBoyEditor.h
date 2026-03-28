#pragma once

#include "MemoryBoyProcessor.h"

//==============================================================================
class MemoryBoyEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MemoryBoyEditor (MemoryBoyProcessor&);
    ~MemoryBoyEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MemoryBoyProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MemoryBoyEditor)
};

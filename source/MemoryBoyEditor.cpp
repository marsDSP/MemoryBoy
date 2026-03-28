#include "MemoryBoyProcessor.h"
#include "MemoryBoyEditor.h"

//==============================================================================
MemoryBoyEditor::MemoryBoyEditor (MemoryBoyProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (400, 300);
}

MemoryBoyEditor::~MemoryBoyEditor()
{
}

//==============================================================================
void MemoryBoyEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void MemoryBoyEditor::resized()
{
}

#include "buffer_static.h"

namespace MarsDSP::Buffers
{
    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::StaticBuffer(int numChannels, int numSamples)
    {
        setMaxSize(numChannels, numSamples);
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    void StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::setMaxSize(int numChannels, int numSamples)
    {
        jassert(numChannels >= 0 && numChannels <= maxNumChannels);
        jassert(numSamples >= 0 && numSamples <= maxNumSamples);

        hasBeenCleared = true;
        currentNumChannels = 0;
        currentNumSamples = 0;

        std::fill(channelPointers.begin(), channelPointers.end(), nullptr);
        for (int ch = 0; ch < numChannels; ++ch)
            channelPointers[static_cast<size_t>(ch)] = channelData[static_cast<size_t>(ch)].data();

        setCurrentSize(numChannels, numSamples);
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    void StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::setCurrentSize(
        int numChannels, int numSamples) noexcept
    {
        const auto increasingNumChannels = numChannels > currentNumChannels;

        if (const auto increasingNumSamples = numSamples > currentNumSamples)
        {
            for (int ch = 0; ch < currentNumChannels; ++ch)
            {
                if (auto *writePointer = channelPointers[static_cast<size_t>(ch)])
                    std::fill(writePointer + currentNumSamples, writePointer + numSamples, SampleType{});
            }
        }

        if (increasingNumChannels)
        {
            for (int ch = currentNumChannels; ch < numChannels; ++ch)
            {
                if (auto *writePointer = channelPointers[static_cast<size_t>(ch)])
                    std::fill(writePointer, writePointer + numSamples, SampleType{});
            }
        }

        currentNumChannels = numChannels;
        currentNumSamples = numSamples;
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    SampleType *StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getWritePointer(int channel) noexcept
    {
        hasBeenCleared = false;
        return channelPointers[static_cast<size_t>(channel)];
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    const SampleType *StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getReadPointer(
        int channel) const noexcept
    {
        return channelPointers[static_cast<size_t>(channel)];
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    std::span<SampleType> StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getWriteSpan(int channel) noexcept
    {
        hasBeenCleared = false;
        return {channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(currentNumSamples)};
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    std::span<const SampleType> StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getReadSpan(
        int channel) const noexcept
    {
        return {channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(currentNumSamples)};
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    SampleType **StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getArrayOfWritePointers() noexcept
    {
        hasBeenCleared = false;
        return channelPointers.data();
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    const SampleType **StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::getArrayOfReadPointers() const noexcept
    {
        return const_cast<const SampleType **>(channelPointers.data());
    }

#if MARSDSP_USING_JUCE
    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    juce::AudioBuffer<SampleType> StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::toAudioBuffer()
    {
        return {getArrayOfWritePointers(), currentNumChannels, currentNumSamples};
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    juce::AudioBuffer<SampleType> StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::toAudioBuffer() const
    {
        return {const_cast<SampleType * const*>(getArrayOfReadPointers()), currentNumChannels, currentNumSamples};
    }

#if JUCE_MODULE_AVAILABLE_juce_dsp
    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    juce::dsp::AudioBlock<SampleType> StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::toAudioBlock()
    {
        return {
            getArrayOfWritePointers(), static_cast<size_t>(currentNumChannels), static_cast<size_t>(currentNumSamples)
        };
    }

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    juce::dsp::AudioBlock<const SampleType> StaticBuffer<SampleType, maxNumChannels,
        maxNumSamples>::toAudioBlock() const
    {
        return {
            getArrayOfReadPointers(), static_cast<size_t>(currentNumChannels), static_cast<size_t>(currentNumSamples)
        };
    }
#endif
#endif

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    void StaticBuffer<SampleType, maxNumChannels, maxNumSamples>::clear() noexcept
    {
        if (hasBeenCleared)
            return;

        for (int ch = 0; ch < currentNumChannels; ++ch)
        {
            if (auto *writePointer = channelPointers[static_cast<size_t>(ch)])
                std::fill(writePointer, writePointer + currentNumSamples, SampleType{});
        }
        hasBeenCleared = true;
    }
}

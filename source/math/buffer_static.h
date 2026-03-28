#pragma once

#ifndef MEMORYBOY_BUFFER_STATIC_H
#define MEMORYBOY_BUFFER_STATIC_H

#include <array>
#include <cstddef>
#include <span>
#include <cassert>
#include <xsimd/xsimd.hpp>
#if MARSDSP_USING_JUCE
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#if JUCE_MODULE_AVAILABLE_juce_dsp
#include <juce_dsp/juce_dsp.h>
#endif
#endif

#ifndef jassert
#define jassert(x) assert(x)
#endif

namespace MarsDSP::Buffers
{
#ifndef DOXYGEN
    namespace buffer_detail
    {
        template<typename ElementType, size_t arraySize, size_t alignment = xsimd::default_arch::alignment()>
        struct AlignedArray
        {
            [[nodiscard]] ElementType *data() noexcept { return array; }
            [[nodiscard]] const ElementType *data() const noexcept { return array; }

        private:
            alignas (alignment) ElementType array[arraySize]{};
        };
    }
#endif

    template<typename SampleType, int maxNumChannels, int maxNumSamples>
    class StaticBuffer
    {
    public:
        using Type = SampleType;

        StaticBuffer() = default;

        StaticBuffer(int numChannels, int numSamples);

        StaticBuffer(StaticBuffer<SampleType, maxNumChannels, maxNumSamples> &&) noexcept = default;

        StaticBuffer<SampleType, maxNumChannels, maxNumSamples> &operator=(StaticBuffer<SampleType, maxNumChannels,
            maxNumSamples> &&) noexcept = default;

        void setMaxSize(int numChannels, int numSamples);

        void setCurrentSize(int numChannels, int numSamples) noexcept;

        void clear() noexcept;

        [[nodiscard]] int getNumChannels() const noexcept { return currentNumChannels; }
        [[nodiscard]] int getNumSamples() const noexcept { return currentNumSamples; }

        [[nodiscard]] SampleType *getWritePointer(int channel) noexcept;

        [[nodiscard]] const SampleType *getReadPointer(int channel) const noexcept;

        [[nodiscard]] std::span<SampleType> getWriteSpan(int channel) noexcept;

        [[nodiscard]] std::span<const SampleType> getReadSpan(int channel) const noexcept;

        [[nodiscard]] SampleType **getArrayOfWritePointers() noexcept;

        [[nodiscard]] const SampleType **getArrayOfReadPointers() const noexcept;

#if MARSDSP_USING_JUCE

        [[nodiscard]] juce::AudioBuffer<SampleType> toAudioBuffer();
        [[nodiscard]] juce::AudioBuffer<SampleType> toAudioBuffer() const;

#if JUCE_MODULE_AVAILABLE_juce_dsp

        [[nodiscard]] juce::dsp::AudioBlock<SampleType> toAudioBlock();
        [[nodiscard]] juce::dsp::AudioBlock<const SampleType> toAudioBlock() const;

#endif
#endif

    private:
        int currentNumChannels = 0;
        int currentNumSamples = 0;
        bool hasBeenCleared = true;
        std::array<buffer_detail::AlignedArray<SampleType, static_cast<size_t>
            (maxNumSamples)>, static_cast<size_t>(maxNumChannels)> channelData{};

        std::array<SampleType *, static_cast<size_t>(maxNumChannels)> channelPointers{};

#if MARSDSP_USING_JUCE
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticBuffer)
#endif
    };
}
#endif

#pragma once

#ifndef MEMORYBOY_BUFFER_HEAD_H
#define MEMORYBOY_BUFFER_HEAD_H

//#include "../../Core/CoreUtils.h"
#include "buffer_detail.h"
#include <vector>
#include <array>
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
#ifndef MARSDSP_BUFFER_MAX_NUM_CHANNELS
#define MARSDSP_BUFFER_MAX_NUM_CHANNELS 32
#endif

    template<typename SampleType, size_t alignment = xsimd::default_arch::alignment()>
    class SIMDBuffer
    {
    public:
        using Type = SampleType;

        SIMDBuffer() = default;

        SIMDBuffer(int numChannels, int numSamples);

        SIMDBuffer(SIMDBuffer &&) noexcept = default;

        SIMDBuffer &operator=(SIMDBuffer &&) noexcept = default;

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
        template<typename T = SampleType>
        std::enable_if_t<buffers_detail::IsFloatOrDouble<T>, juce::AudioBuffer<SampleType> > toAudioBuffer()
        {
            return {getArrayOfWritePointers(), currentNumChannels, currentNumSamples};
        }

        template<typename T = SampleType>
        std::enable_if_t<buffers_detail::IsFloatOrDouble<T>, juce::AudioBuffer<SampleType> > toAudioBuffer() const
        {
            return {const_cast<SampleType * const*>(getArrayOfReadPointers()), currentNumChannels, currentNumSamples};
        }

#if JUCE_MODULE_AVAILABLE_juce_dsp
        template<typename T = SampleType>
        std::enable_if_t<buffers_detail::IsFloatOrDouble<T>, juce::dsp::AudioBlock<SampleType> > toAudioBlock()
        {
            return {
                getArrayOfWritePointers(), static_cast<size_t>(currentNumChannels),
                static_cast<size_t>(currentNumSamples)
            };
        }

        template<typename T = SampleType>
        std::enable_if_t<buffers_detail::IsFloatOrDouble<T>, juce::dsp::AudioBlock<const SampleType> >
        toAudioBlock() const
        {
            return {
                getArrayOfReadPointers(), static_cast<size_t>(currentNumChannels),
                static_cast<size_t>(currentNumSamples)
            };
        }
#endif
#endif

    private:
        int currentNumChannels = 0;
        int currentNumSamples = 0;
        bool hasBeenCleared = true;

        static constexpr int maxNumChannels = MARSDSP_BUFFER_MAX_NUM_CHANNELS;

        std::vector<SampleType, xsimd::aligned_allocator<SampleType, alignment> > rawData;
        std::array<SampleType *, maxNumChannels> channelPointers{};

#if MARSDSP_USING_JUCE
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIMDBuffer)
#endif
    };

    template<typename SampleType, size_t alignment = xsimd::default_arch::alignment()>
    using Buffer = SIMDBuffer<SampleType, alignment>;
}
#endif

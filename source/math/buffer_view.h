#pragma once

#ifndef MEMORYBOY_BUFFER_VIEW_H
#define MEMORYBOY_BUFFER_VIEW_H

#include <span>
#include <vector>
#include <array>
#include <cstddef>
#include <type_traits>
#include <xsimd/xsimd.hpp>
#include <JuceHeader.h>
#include "buffer_head.h"
#include "buffer_static.h"
#include "buffer_detail.h"
#if MARSDSP_USING_JUCE
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#if JUCE_MODULE_AVAILABLE_juce_dsp
#include <juce_dsp/juce_dsp.h>
#include "simd/SIMDAudioBlock.h"
#endif
#endif

#ifndef jassert
#include <cassert>
#define jassert(x) assert(x)
#endif

#if JUCE_MODULE_AVAILABLE_DataStructures
#include <DataStructures/DataStructures.h>
#endif

namespace MarsDSP::Buffers
{
    template<typename SampleType>
    class BufferView
    {
    public:
        using Type = SampleType;

        BufferView() = default;

        BufferView &operator=(const BufferView &) = default;

        BufferView(BufferView &&) noexcept = default;

        BufferView &operator=(BufferView &&) noexcept = default;

        BufferView(SampleType *const*data, int dataNumChannels, int dataNumSamples,
                   int sampleOffset = 0) : numChannels(dataNumChannels),
                                           numSamples(dataNumSamples)
        {
            initialise(data, sampleOffset);
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> >* = nullptr>
        BufferView(const SampleType *const*data, int dataNumChannels, int dataNumSamples,
                   int sampleOffset = 0) : numChannels(dataNumChannels),
                                           numSamples(dataNumSamples)
        {
            initialise(data, sampleOffset);
        }

        BufferView(SampleType *data, int dataNumSamples, int sampleOffset = 0) : numSamples(dataNumSamples)
        {
            initialise(&data, sampleOffset);
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> >* = nullptr>
        BufferView(const SampleType *data, int dataNumSamples, int sampleOffset = 0) : numSamples(dataNumSamples)
        {
            initialise(&data, sampleOffset);
        }

        BufferView(SIMDBuffer<std::remove_const_t<SampleType> > &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfWritePointers(), sampleOffset, startChannel);
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> >* = nullptr>
        BufferView(const SIMDBuffer<std::remove_const_t<SampleType> > &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfReadPointers(), sampleOffset, startChannel);
        }

        template<int maxNumChannels, int maxNumSamples>
        BufferView(StaticBuffer<std::remove_const_t<SampleType>, maxNumChannels, maxNumSamples> &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfWritePointers(), sampleOffset, startChannel);
        }

        template<int maxNumChannels, int maxNumSamples, typename T = SampleType, std::enable_if_t<std::is_const_v<T> >*
                = nullptr>
        BufferView(const StaticBuffer<std::remove_const_t<SampleType>, maxNumChannels, maxNumSamples> &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfReadPointers(), sampleOffset, startChannel);
        }

        BufferView(const BufferView<std::remove_const_t<SampleType> > &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfWritePointers(), sampleOffset, startChannel);
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> >* = nullptr>
        BufferView(const BufferView<const SampleType> &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            jassert(buffer.getNumChannels() >= startChannel + numChannels);
            jassert(buffer.getNumSamples() >= sampleOffset + numSamples);
            initialise(buffer.getArrayOfReadPointers(), sampleOffset, startChannel);
        }

#if MARSDSP_USING_JUCE
        BufferView(AudioBuffer<std::remove_const_t<SampleType> > &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            initialise(buffer.getArrayOfWritePointers(), sampleOffset, startChannel);
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> >* = nullptr>
        BufferView(const AudioBuffer<std::remove_const_t<SampleType> > &buffer,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0 ? (buffer.getNumChannels() - startChannel) : bufferNumChannels),
              numSamples(bufferNumSamples < 0 ? (buffer.getNumSamples() - sampleOffset) : bufferNumSamples)
        {
            initialise(buffer.getArrayOfReadPointers(), sampleOffset, startChannel);
        }

        [[nodiscard]] AudioBuffer<SampleType> toAudioBuffer() const noexcept
        {
            return {channelPointers.data(), numChannels, numSamples};
        }

#if JUCE_MODULE_AVAILABLE_juce_dsp
        template<typename T = SampleType, std::enable_if_t<std::is_floating_point_v<T> >* = nullptr>
        BufferView(const Utils::Buffers::AudioBlock<std::remove_const_t<SampleType> > &block,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0
                              ? (static_cast<int>(block.getNumChannels()) - startChannel)
                              : bufferNumChannels),
              numSamples(bufferNumSamples < 0
                             ? (static_cast<int>(block.getNumSamples()) - sampleOffset)
                             : bufferNumSamples)
        {
            for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
                channelPointers[ch] = block.getChannelPointer(ch + static_cast<size_t>(startChannel)) + sampleOffset;
        }

        template<typename T = SampleType, std::enable_if_t<std::is_const_v<T> && std::is_floating_point_v<T>>* =
                nullptr>
        BufferView(const Utils::Buffers::AudioBlock<const SampleType> &block,
                   int sampleOffset = 0,
                   int bufferNumSamples = -1,
                   int startChannel = 0,
                   int bufferNumChannels = -1)
            : numChannels(bufferNumChannels < 0
                              ? (static_cast<int>(block.getNumChannels()) - startChannel)
                              : bufferNumChannels),
              numSamples(bufferNumSamples < 0
                             ? (static_cast<int>(block.getNumSamples()) - sampleOffset)
                             : bufferNumSamples)
        {
            for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
                channelPointers[ch] = block.getChannelPointer(ch + static_cast<size_t>(startChannel)) + sampleOffset;
        }

        [[nodiscard]] Utils::Buffers::AudioBlock<SampleType> toAudioBlock() const noexcept
        {
            return {channelPointers.data(), static_cast<size_t>(numChannels), static_cast<size_t>(numSamples)};
        }
#endif
#endif

        template<typename T = SampleType>
        std::enable_if_t<!std::is_const_v<T>, void> clear() const noexcept
        {
            buffers_detail::clear(const_cast<SampleType **>(channelPointers.data()), 0, numChannels, 0, numSamples);
        }

        [[nodiscard]] int getNumChannels() const noexcept { return numChannels; }

        [[nodiscard]] int getNumSamples() const noexcept { return numSamples; }

        template<typename T = SampleType>
        [[nodiscard]] std::enable_if_t<!std::is_const_v<T>, SampleType *> getWritePointer(int channel) const noexcept
        {
            return channelPointers[static_cast<size_t>(channel)];
        }

        [[nodiscard]] const SampleType *getReadPointer(int channel) const noexcept
        {
            return channelPointers[static_cast<size_t>(channel)];
        }

        template<typename T = SampleType>
        [[nodiscard]] std::enable_if_t<!std::is_const_v<T>, std::span<SampleType> > getWriteSpan(
            int channel) const noexcept
        {
            return {channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(numSamples)};
        }

        [[nodiscard]] std::span<const SampleType> getReadSpan(int channel) const noexcept
        {
            return {channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(numSamples)};
        }

        template<typename T = SampleType>
        [[nodiscard]] std::enable_if_t<!std::is_const_v<T>, SampleType * const*>
        getArrayOfWritePointers() const noexcept
        {
            return channelPointers.data();
        }

        [[nodiscard]] const SampleType *const*getArrayOfReadPointers() const noexcept
        {
            return const_cast<const SampleType * const*>(channelPointers.data());
        }

    private:
        template<typename T = SampleType>
        std::enable_if_t<!std::is_const_v<T>, void> initialise(SampleType *const*data, int sampleOffset,
                                                               int startChannel = 0)
        {
            jassert(juce::isPositiveAndNotGreaterThan (numChannels, maxNumChannels));
            jassert(numSamples >= 0);
            for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
                channelPointers[ch] = data[ch + static_cast<size_t>(startChannel)] + sampleOffset;
        }

        template<typename T = SampleType>
        std::enable_if_t<std::is_const_v<T>, void> initialise(const SampleType *const*data, int sampleOffset,
                                                              int startChannel = 0)
        {
            jassert(juce::isPositiveAndNotGreaterThan (numChannels, maxNumChannels));
            jassert(numSamples >= 0);
            for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
                channelPointers[ch] = data[ch + static_cast<size_t>(startChannel)] + sampleOffset;
        }

        int numChannels = 1;
        int numSamples = 0;

        static constexpr int maxNumChannels = MARSDSP_BUFFER_MAX_NUM_CHANNELS;
        std::array<SampleType *, static_cast<size_t>(maxNumChannels)> channelPointers{};
    };

#ifndef DOXYGEN
    namespace detail
    {
        template<typename BufferType, typename SampleType> // investigate why these are never used
        struct is_static_buffer
        {
            static constexpr bool value = false;
        };

        template<typename T, int nChannels, int nSamples>
        struct is_static_buffer<StaticBuffer<T, nChannels, nSamples>, T>
        {
            static constexpr bool value = true;
        };

        template<typename BufferType, typename SampleType>
        constexpr auto is_static_buffer_v = is_static_buffer<BufferType, SampleType>::value;

        static_assert(is_static_buffer_v<StaticBuffer<float, 1, 1>, float> == true);
        static_assert(is_static_buffer_v<StaticBuffer<float, 1, 1>, double> != true);
    }
#endif

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, SIMDBuffer<float> > || detail::is_static_buffer_v<BufferType, float>
#if MARSDSP_USING_JUCE
            || std::is_same_v<BufferType, AudioBuffer<float> >
#if JUCE_MODULE_AVAILABLE_juce_dsp
            || std::is_same_v<BufferType, dsp::AudioBlock<float> >
#endif
#endif
        > >
    BufferView(BufferType &, Ts...) -> BufferView<float>;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, const SIMDBuffer<float>> || (
                std::is_const_v<BufferType> && detail::is_static_buffer_v<std::remove_const_t<BufferType>, float>)
#if MARSDSP_USING_JUCE
            || std::is_same_v<BufferType, const AudioBuffer<float>>
#if JUCE_MODULE_AVAILABLE_juce_dsp
            || std::is_same_v<std::remove_const_t<BufferType>, dsp::AudioBlock<const float> >
#endif
#endif
        > >
    BufferView(BufferType &, Ts...) -> BufferView<const float>;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, SIMDBuffer<double> > || detail::is_static_buffer_v<BufferType, double>
#if MARSDSP_USING_JUCE
            || std::is_same_v<BufferType, AudioBuffer<double> >
#if JUCE_MODULE_AVAILABLE_juce_dsp
            || std::is_same_v<BufferType, dsp::AudioBlock<double> >
#endif
#endif
        > >
    BufferView(BufferType &, Ts...) -> BufferView<double>;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, const SIMDBuffer<double>> || (
                std::is_const_v<BufferType> && detail::is_static_buffer_v<std::remove_const_t<BufferType>, double>)
#if MARSDSP_USING_JUCE
            || std::is_same_v<BufferType, const AudioBuffer<double>>
#if JUCE_MODULE_AVAILABLE_juce_dsp
            || std::is_same_v<std::remove_const_t<BufferType>, dsp::AudioBlock<const double> >
#endif
#endif
        > >
    BufferView(BufferType &, Ts...) -> BufferView<const double>;

#if ! MARSDSP_NO_XSIMD

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, SIMDBuffer<xsimd::batch<float> > > || detail::is_static_buffer_v<BufferType,
                xsimd::batch<float> >> >
    BufferView(BufferType &, Ts...) -> BufferView<xsimd::batch<float> >;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, const SIMDBuffer<xsimd::batch<float> >> || (
                std::is_const_v<BufferType> && detail::is_static_buffer_v<std::remove_const_t<BufferType>, xsimd::batch<
                    float> >)> >
    BufferView(BufferType &, Ts...) -> BufferView<const xsimd::batch<float>>;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, SIMDBuffer<xsimd::batch<double> > > || detail::is_static_buffer_v<BufferType,
                xsimd::batch<double> >> >
    BufferView(BufferType &, Ts...) -> BufferView<xsimd::batch<double> >;

    template<typename BufferType,
        typename... Ts,
        typename = std::enable_if_t<
            std::is_same_v<BufferType, const SIMDBuffer<xsimd::batch<double> >> || (
                std::is_const_v<BufferType> && detail::is_static_buffer_v<std::remove_const_t<BufferType>, xsimd::batch<
                    double> >)> >
    BufferView(BufferType &, Ts...) -> BufferView<const xsimd::batch<double>>;
#endif

#if JUCE_MODULE_AVAILABLE_DataStructures
    template<typename T>
    BufferView<T> make_temp_buffer(ArenaAllocatorView arena, int num_channels, int num_samples)
    {
        int num_samples_padded = num_samples;
#if ! MARSDSP_NO_XSIMD
    static constexpr auto vec_size = static_cast<int>(xsimd::batch<T>::size);
        if constexpr (std::is_floating_point_v<T>)
    num_samples_padded= buffers_detail::ceiling_divide(num_samples, vec_size) *vec_size;
#endif

    std::array<T *, MARSDSP_BUFFER_MAX_NUM_CHANNELS> channel_pointers{};
        for (size_t ch =0; ch<static_cast<size_t>(num_channels);++ch)
        {
            channel_pointers[ch] = arena.allocate<T> (num_samples_padded, xsimd::default_arch::alignment());
            jassert (channel_pointers[ch] != nullptr);
        }
        return { channel_pointers.data(), num_channels, num_samples };
    }
#endif
}
#endif

#pragma once

#ifndef MEMORYBOY_BUFFER_HELPER_SIMD_H
#define MEMORYBOY_BUFFER_HELPER_SIMD_H

#include <xsimd/xsimd.hpp>

namespace MarsDSP::Buffers
{
#if ! MARSDSP_NO_XSIMD

    template<typename ScalarSampleType, typename SIMDSampleType = ScalarSampleType>
    [[maybe_unused]] static void copyToSIMDBuffer(const BufferView<const ScalarSampleType> &scalarBuffer,
                                                  const BufferView<xsimd::batch<SIMDSampleType> > &simdBuffer) noexcept
    {
        using Vec = xsimd::batch<SIMDSampleType>;
        static constexpr auto vecSize = static_cast<int>(Vec::size);

        const auto numSamples = scalarBuffer.getNumSamples();
        const auto numScalarChannels = scalarBuffer.getNumChannels();

        const auto interleaveSamples = [numSamples](const ScalarSampleType **sourceChannels,
                                                    SIMDSampleType *destinationInterleaved, int numberOfChannels)
        {
            std::fill(destinationInterleaved, destinationInterleaved + numSamples * vecSize,
                      static_cast<SIMDSampleType>(0));

            for (int channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex)
            {
                auto interleavedIndex = channelIndex;
                const auto *channelData = sourceChannels[channelIndex];

                for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
                {
                    destinationInterleaved[interleavedIndex] = static_cast<SIMDSampleType>(channelData[sampleIndex]);
                    interleavedIndex += vecSize;
                }
            }
        };

        for (int ch = 0; ch < numScalarChannels; ch += vecSize)
        {
            const auto channelsToInterleave = juce::jmin(vecSize, numScalarChannels - ch);

            const ScalarSampleType *scalarChannelPointers[static_cast<size_t>(vecSize)]{};
            for (int i = 0; i < channelsToInterleave; ++i)
            {
                const auto scalarChannelIndex = ch + i;
                scalarChannelPointers[i] = scalarBuffer.getReadPointer(scalarChannelIndex);
            }

            auto *simdChannelData = reinterpret_cast<SIMDSampleType *>(simdBuffer.getWritePointer(ch / vecSize));
            interleaveSamples(scalarChannelPointers, simdChannelData, channelsToInterleave);
        }
    }


    template<typename ScalarSampleType, typename SIMDSampleType = ScalarSampleType>
    [[maybe_unused]] static void copyToSIMDBuffer(const BufferView<const ScalarSampleType> &scalarBuffer,
                                                  SIMDBuffer<xsimd::batch<SIMDSampleType> > &simdBuffer) noexcept
    {
        using Vec = xsimd::batch<SIMDSampleType>;
        static constexpr auto vecSize = static_cast<int>(Vec::size);

        const auto numSamples = scalarBuffer.getNumSamples();
        const auto numScalarChannels = scalarBuffer.getNumChannels();
        const auto numSIMDChannels = buffers_detail::ceiling_divide(numScalarChannels, vecSize);

        simdBuffer.setCurrentSize(numSIMDChannels, numSamples);

        copyToSIMDBuffer(scalarBuffer, BufferView{simdBuffer});
    }

    template<typename ScalarSampleType, typename SIMDSampleType = ScalarSampleType>
    [[maybe_unused]] static void copyToSIMDBuffer(const SIMDBuffer<ScalarSampleType> &scalarBuffer,
                                                  SIMDBuffer<xsimd::batch<SIMDSampleType> > &simdBuffer) noexcept
    {
        copyToSIMDBuffer(static_cast<const BufferView<const ScalarSampleType> &>(scalarBuffer), simdBuffer);
    }

    template<typename ScalarSampleType, typename SIMDSampleType = ScalarSampleType>
    [[maybe_unused]] static void copyFromSIMDBuffer(const BufferView<const xsimd::batch<SIMDSampleType>> &simdBuffer,
                                                    const BufferView<ScalarSampleType> &scalarBuffer) noexcept
    {
        using Vec = xsimd::batch<SIMDSampleType>;
        static constexpr auto vecSize = static_cast<int>(Vec::size);

        const auto numSamples = simdBuffer.getNumSamples();
        const auto numSIMDChannels = simdBuffer.getNumChannels();

        jassert(scalarBuffer.getNumSamples() == numSamples);
        jassert(scalarBuffer.getNumChannels() <= numSIMDChannels * vecSize);

        const auto deinterleaveSamples = [numSamples](const SIMDSampleType *sourceInterleaved,
                                                      ScalarSampleType **destinationChannels, int numberOfChannels)
        {
            for (int channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex)
            {
                auto interleavedIndex = channelIndex;
                auto *channelData = destinationChannels[channelIndex];

                for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
                {
                    channelData[sampleIndex] = static_cast<ScalarSampleType>(sourceInterleaved[interleavedIndex]);
                    interleavedIndex += vecSize;
                }
            }
        };

        for (int ch = 0; ch < numSIMDChannels; ++ch)
        {
            const auto channelsToDeinterleave = std::min(vecSize, scalarBuffer.getNumChannels() - ch * vecSize);

            ScalarSampleType *scalarChannelPointers[static_cast<size_t>(vecSize)]{};
            for (int i = 0; i < channelsToDeinterleave; ++i)
            {
                const auto scalarChannelIndex = ch * vecSize + i;
                scalarChannelPointers[i] = scalarBuffer.getWritePointer(scalarChannelIndex);
            }

            auto *simdChannelData = reinterpret_cast<const SIMDSampleType *>(simdBuffer.getReadPointer(ch));
            deinterleaveSamples(simdChannelData, scalarChannelPointers, channelsToDeinterleave);
        }
    }

    template<typename ScalarSampleType, typename SIMDSampleType = ScalarSampleType>
    [[maybe_unused]] static void copyFromSIMDBuffer(const SIMDBuffer<xsimd::batch<SIMDSampleType> > &simdBuffer,
                                                    SIMDBuffer<ScalarSampleType> &scalarBuffer) noexcept
    {
        copyFromSIMDBuffer<ScalarSampleType, SIMDSampleType>(
            simdBuffer, static_cast<const BufferView<ScalarSampleType> &>(scalarBuffer));
    }
#endif
}
#endif

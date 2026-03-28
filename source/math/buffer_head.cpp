#include "buffer_head.h"

namespace MarsDSP::Buffers
{
        template <typename SampleType, size_t alignment>
    SIMDBuffer<SampleType, alignment>::SIMDBuffer (int numChannels, int numSamples)
    {
        setMaxSize (numChannels, numSamples);
    }

    template <typename SampleType, size_t alignment>
    void SIMDBuffer<SampleType, alignment>::setMaxSize (int numChannels, int numSamples)
    {

        jassert (numChannels >= 0 && numChannels <= maxNumChannels);

        numChannels = std::max (numChannels, 1);
        numSamples = std::max (numSamples, 0);

        int numSamplesPadded = numSamples;
    #if ! MARSDSP_NO_XSIMD
        static constexpr auto vectorSize = static_cast<int>(xsimd::batch<SampleType>::size);
        if constexpr (std::is_floating_point_v<SampleType>)
            numSamplesPadded = ceiling_divide (numSamples, vectorSize) * vectorSize;
    #endif

        rawData.clear();
        hasBeenCleared = true;
        currentNumChannels = 0;
        currentNumSamples = 0;

        rawData.resize (static_cast<size_t>(numChannels) * static_cast<size_t>(numSamplesPadded), SampleType {});
        std::fill (channelPointers.begin(), channelPointers.end(), nullptr);
        for (int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
            channelPointers[static_cast<size_t>(channelIndex)] = rawData.data() + channelIndex * numSamplesPadded;

        setCurrentSize (numChannels, numSamples);
    }

    template <typename SampleType, size_t alignment>
    void SIMDBuffer<SampleType, alignment>::setCurrentSize (int numChannels, int numSamples) noexcept
    {

        jassert (numSamples * numChannels <= static_cast<int> (rawData.size()));

        const auto increasingNumChannels = numChannels > currentNumChannels;
        const auto increasingNumSamples = numSamples > currentNumSamples;

        if (increasingNumSamples)
            buffers_detail::clear (channelPointers.data(), 0, currentNumChannels, currentNumSamples, numSamples);

        if (increasingNumChannels)
            buffers_detail::clear (channelPointers.data(), currentNumChannels, numChannels, 0, numSamples);

        currentNumChannels = numChannels;
        currentNumSamples = numSamples;
    }

    template <typename SampleType, size_t alignment>
    SampleType* SIMDBuffer<SampleType, alignment>::getWritePointer (int channel) noexcept
    {
        hasBeenCleared = false;
        return channelPointers[static_cast<size_t>(channel)];
    }

    template <typename SampleType, size_t alignment>
    const SampleType* SIMDBuffer<SampleType, alignment>::getReadPointer (int channel) const noexcept
    {
        return channelPointers[static_cast<size_t>(channel)];
    }

    template <typename SampleType, size_t alignment>
    std::span<SampleType> SIMDBuffer<SampleType, alignment>::getWriteSpan (int channel) noexcept
    {
        hasBeenCleared = false;
        return { channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(currentNumSamples) };
    }

    template <typename SampleType, size_t alignment>
    std::span<const SampleType> SIMDBuffer<SampleType, alignment>::getReadSpan (int channel) const noexcept
    {
        return { channelPointers[static_cast<size_t>(channel)], static_cast<size_t>(currentNumSamples) };
    }

    template <typename SampleType, size_t alignment>
    SampleType** SIMDBuffer<SampleType, alignment>::getArrayOfWritePointers() noexcept
    {
        hasBeenCleared = false;
        return channelPointers.data();
    }

    template <typename SampleType, size_t alignment>
    const SampleType** SIMDBuffer<SampleType, alignment>::getArrayOfReadPointers() const noexcept
    {
        return const_cast<const SampleType**> (channelPointers.data());
    }

    template <typename SampleType, size_t alignment>
    void SIMDBuffer<SampleType, alignment>::clear() noexcept
    {
        if (hasBeenCleared)
            return;

        buffers_detail::clear (channelPointers.data(), 0, currentNumChannels, 0, currentNumSamples);
        hasBeenCleared = true;
    }

    #if MARSDSP_ALLOW_TEMPLATE_INSTANTIATIONS
    template class SIMDBuffer<float>;
    template class SIMDBuffer<double>;

    #if ! MARSDSP_NO_XSIMD
    template class SIMDBuffer<xsimd::batch<float>>;
    template class SIMDBuffer<xsimd::batch<double>>;
    #endif
    #endif
}
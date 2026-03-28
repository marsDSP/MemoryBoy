#pragma once

#ifndef MEMORYBOY_BUFFER_HELPERS_H
#define MEMORYBOY_BUFFER_HELPERS_H

#include <type_traits>
#include "buffer_head.h"
#include "buffer_static.h"
#include "buffer_view.h"

namespace MarsDSP::Buffers
{
#ifndef DOXYGEN
    namespace detail
    {
        template<typename BufferType>
        struct BufferInfoBase
        {
            using sample_type = std::remove_const_t<typename BufferType::Type>;
            using const_type = const BufferType;
            static constexpr bool is_buffer = true;
        };

        template<typename BufferType>
        struct BufferInfo
        {
            static constexpr bool is_buffer = false;
        };

        template<typename SampleType>
        struct BufferInfo<SIMDBuffer<SampleType> > : BufferInfoBase<SIMDBuffer<SampleType> >
        {
        };

        template<typename SampleType>
        struct BufferInfo<const SIMDBuffer<SampleType>> : BufferInfoBase<SIMDBuffer<SampleType> >
        {
        };

        template<typename SampleType, int nChannels, int nSamples>
        struct BufferInfo<StaticBuffer<SampleType, nChannels,
                    nSamples> > : BufferInfoBase<StaticBuffer<SampleType, nChannels, nSamples> >
        {
        };

        template<typename SampleType, int nChannels, int nSamples>
        struct BufferInfo<const StaticBuffer<SampleType, nChannels,
                    nSamples>> : BufferInfoBase<StaticBuffer<SampleType, nChannels, nSamples> >
        {
        };

        template<typename SampleType>
        struct BufferInfo<BufferView<SampleType> > : BufferInfoBase<BufferView<SampleType> >
        {
            using const_type = BufferView<const std::remove_const_t<SampleType>>;
        };

        template<typename SampleType>
        struct BufferInfo<const BufferView<SampleType>> : BufferInfoBase<BufferView<SampleType> >
        {
            using const_type = const BufferView<const std::remove_const_t<SampleType>>;
        };

#if MARSDSP_USING_JUCE
        template<typename SampleType>
        struct BufferInfo<AudioBuffer<SampleType> >
        {
            using sample_type = SampleType;
            using const_type = const AudioBuffer<SampleType>;
            static constexpr bool is_buffer = true;
        };

        template<typename SampleType>
        struct BufferInfo<const AudioBuffer<SampleType>>
        {
            using sample_type = SampleType;
            using const_type = const AudioBuffer<SampleType>;
            static constexpr bool is_buffer = true;
        };
#endif

        static_assert(BufferInfo<std::string>::is_buffer == false);
        static_assert(BufferInfo<SIMDBuffer<float> >::is_buffer == true);
        static_assert(BufferInfo<StaticBuffer<float, 1, 10> >::is_buffer == true);
        static_assert(BufferInfo<BufferView<float> >::is_buffer == true);

        static_assert(std::is_same_v<BufferInfo<SIMDBuffer<float> >::sample_type, float>);
        static_assert(std::is_same_v<BufferInfo<SIMDBuffer<double> >::sample_type, double>);
        static_assert(std::is_same_v<BufferInfo<SIMDBuffer<float> >::const_type, const SIMDBuffer<float>>);
        static_assert(std::is_same_v<BufferInfo<const SIMDBuffer<float>>::const_type, const SIMDBuffer<float>>);
#if ! MARSDSP_NO_XSIMD
        static_assert(std::is_same_v<BufferInfo<SIMDBuffer<xsimd::batch<float> > >::sample_type, xsimd::batch<float> >);
        static_assert(
            std::is_same_v<BufferInfo<SIMDBuffer<xsimd::batch<double> > >::sample_type, xsimd::batch<double> >);
#endif

        static_assert(std::is_same_v<BufferInfo<StaticBuffer<float, 1, 10> >::sample_type, float>);
        static_assert(std::is_same_v<BufferInfo<StaticBuffer<double, 1, 10> >::sample_type, double>);
        static_assert(
            std::is_same_v<BufferInfo<StaticBuffer<float, 1, 10> >::const_type, const StaticBuffer<float, 1, 10>>);
        static_assert(
            std::is_same_v<BufferInfo<const StaticBuffer<float, 1, 10>>::const_type, const StaticBuffer<float, 1, 10>>);
#if ! MARSDSP_NO_XSIMD
        static_assert(
            std::is_same_v<BufferInfo<StaticBuffer<xsimd::batch<float>, 1, 10> >::sample_type, xsimd::batch<float> >);
        static_assert(
            std::is_same_v<BufferInfo<StaticBuffer<xsimd::batch<double>, 1, 10> >::sample_type, xsimd::batch<double> >);
#endif

        static_assert(std::is_same_v<BufferInfo<BufferView<float> >::sample_type, float>);
        static_assert(std::is_same_v<BufferInfo<BufferView<double> >::sample_type, double>);
        static_assert(std::is_same_v<BufferInfo<BufferView<float> >::const_type, BufferView<const float> >);
        static_assert(std::is_same_v<BufferInfo<const BufferView<float>>::const_type, const BufferView<const float>>);
        static_assert(std::is_same_v<BufferInfo<BufferView<const float> >::const_type, BufferView<const float> >);
        static_assert(
            std::is_same_v<BufferInfo<const BufferView<const float>>::const_type, const BufferView<const float>>);
#if ! MARSDSP_NO_XSIMD
        static_assert(std::is_same_v<BufferInfo<SIMDBuffer<xsimd::batch<float> > >::sample_type, xsimd::batch<float> >);
        static_assert(
            std::is_same_v<BufferInfo<SIMDBuffer<xsimd::batch<double> > >::sample_type, xsimd::batch<double> >);
#endif

#if MARSDSP_USING_JUCE
        static_assert(std::is_same_v<BufferInfo<AudioBuffer<float> >::sample_type, float>);
        static_assert(std::is_same_v<BufferInfo<AudioBuffer<double> >::sample_type, double>);
        static_assert(std::is_same_v<BufferInfo<AudioBuffer<float> >::const_type, const AudioBuffer<float>>)
        ;
        static_assert(
            std::is_same_v<BufferInfo<const AudioBuffer<float>>::const_type, const AudioBuffer<float>>);
#endif
    }
#endif

    template<typename BufferType>
    using ConstBufferType = detail::BufferInfo<std::remove_reference_t<BufferType> >::const_type;

    template<typename BufferType>
    static constexpr bool IsConstBuffer = std::is_same_v<ConstBufferType<BufferType>, std::remove_reference_t<
        BufferType> >;

    template<typename BufferType>
    using BufferSampleType = detail::BufferInfo<BufferType>::sample_type;
}
#endif //MEMORYBOY_BUFFER_HELPERS_H

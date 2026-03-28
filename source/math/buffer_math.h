#pragma once

#ifndef MEMORYBOY_BUFFER_MATH_H
#define MEMORYBOY_BUFFER_MATH_H

#include <juce_core/juce_core.h>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include "buffer_core.h"

#if !defined(MARSDSP_NO_XSIMD)
 #include <xsimd/xsimd.hpp>
#endif
#ifndef MARSDSP_USING_XSIMD_STD
 #if defined(MARSDSP_NO_XSIMD)
  #define MARSDSP_USING_XSIMD_STD(func) using std::func
 #else
  #define MARSDSP_USING_XSIMD_STD(func) using xsimd::func
 #endif
#endif

namespace MarsDSP::Math {
inline namespace BufferMath
{
    using namespace juce;
    using Buffers::BufferSampleType;

    struct SampleTypeHelpers
    {
        template <typename T>
        using NumericType = dsp::SampleTypeHelpers::ElementType<T>::Type;

    #if !defined(MARSDSP_NO_XSIMD)
        template <typename T>
        static constexpr bool IsSIMDRegister = xsimd::is_batch<T>::value;
    #else
        template <typename T>
        static constexpr bool IsSIMDRegister = false;
    #endif
    };

    struct SIMDUtils
    {
#if !defined(MARSDSP_NO_XSIMD)
        static constexpr std::size_t defaultSIMDAlignment = xsimd::default_arch::alignment();
#else
        static constexpr std::size_t defaultSIMDAlignment = 16;
#endif

        template <typename T>
        static bool isAligned (const T* ptr)
        {
        #if !defined(MARSDSP_NO_XSIMD)
            return xsimd::is_aligned(ptr);
        #else
            return (reinterpret_cast<uintptr_t>(ptr) % 16 == 0);
        #endif
        }
    };

    struct FloatVectorOperations : juce::FloatVectorOperations
    {

        using juce::FloatVectorOperations::add;
        using juce::FloatVectorOperations::subtract;
        using juce::FloatVectorOperations::multiply;
        using juce::FloatVectorOperations::copy;
        using juce::FloatVectorOperations::clear;
        using juce::FloatVectorOperations::fill;
        using juce::FloatVectorOperations::negate;
        using juce::FloatVectorOperations::abs;
        using juce::FloatVectorOperations::min;
        using juce::FloatVectorOperations::max;
        using juce::FloatVectorOperations::clip;
        using juce::FloatVectorOperations::findMinimum;
        using juce::FloatVectorOperations::findMaximum;
        using juce::FloatVectorOperations::findMinAndMax;

        static float findAbsoluteMaximum (const float* src, int numSamples)
        {
            auto minMax = juce::FloatVectorOperations::findMinAndMax (src, numSamples);
            return std::max (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));
        }

        static float computeRMS (const float* src, int numSamples)
        {
            if (numSamples <= 0) return 0.0f;
            double sum = 0.0;
            for (int i = 0; i < numSamples; ++i)
                sum += static_cast<double> (src[i]) * static_cast<double> (src[i]);
            return static_cast<float> (std::sqrt (sum / static_cast<double> (numSamples)));
        }

        static int countInfsAndNaNs (const float* src, int numSamples)
        {
            int count = 0;
            for (int i = 0; i < numSamples; ++i)
                if (! std::isfinite (src[i])) count++;
            return count;
        }
    };

    template <typename BufferType>
    auto getMagnitude (const BufferType& buffer, int startSample = 0, int numSamples = -1, int channel = -1) noexcept;

    template <typename BufferType>
    auto getRMSLevel (const BufferType& buffer, int channel, int startSample = 0, int numSamples = -1) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1>
    void copyBufferData (const BufferType1& bufferSrc, BufferType2& bufferDest, int srcStartSample = 0, int destStartSample = 0, int numSamples = -1, int startChannel = 0, int numChannels = -1) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1>
    void copyBufferChannels (const BufferType1& bufferSrc, BufferType2& bufferDest, int srcChannel, int destChannel) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1>
    void addBufferData (const BufferType1& bufferSrc, BufferType2& bufferDest, int srcStartSample = 0, int destStartSample = 0, int numSamples = -1, int startChannel = 0, int numChannels = -1) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1>
    void addBufferChannels (const BufferType1& bufferSrc, BufferType2& bufferDest, int srcChannel, int destChannel) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1>
    void multiplyBufferData (const BufferType1& bufferSrc, BufferType2& bufferDest, int srcStartSample = 0, int destStartSample = 0, int numSamples = -1, int startChannel = 0, int numChannels = -1) noexcept;

    template <typename BufferType, typename FloatType = BufferType::Type>
    void applyGain (BufferType& buffer, FloatType gain) noexcept;

    template <typename BufferType1, typename BufferType2 = BufferType1, typename FloatType = BufferType1::Type>
    void applyGain (const BufferType1& bufferSrc, BufferType2& bufferDest, FloatType gain) noexcept;

    template <typename BufferType, typename SmoothedValueType>
    void applyGainSmoothed (BufferType& buffer, SmoothedValueType& gain) noexcept;

    template <typename BufferType1, typename SmoothedValueType, typename BufferType2 = BufferType1>
    void applyGainSmoothed (const BufferType1& bufferSrc, BufferType2& bufferDest, SmoothedValueType& gain) noexcept;

    template <typename BufferType, typename SmoothedBufferType>
    void applyGainSmoothedBuffer (BufferType& buffer, SmoothedBufferType& gain) noexcept;

    template <typename BufferType1, typename SmoothedBufferType, typename BufferType2 = BufferType1>
    void applyGainSmoothedBuffer (const BufferType1& bufferSrc, BufferType2& bufferDest, SmoothedBufferType& gain) noexcept;

    template <typename BufferType1, typename BufferType2, typename FloatType = SampleTypeHelpers::NumericType<BufferSampleType<BufferType1>>>
    void sumToMono (const BufferType1& bufferSrc, BufferType2& bufferDest, FloatType normGain = static_cast<FloatType>(-1));

    template <typename BufferType, typename FloatType = BufferSampleType<BufferType>>
    bool sanitizeBuffer (BufferType& buffer, FloatType ceiling = static_cast<FloatType>(100)) noexcept;

    template <typename BufferType, typename FunctionType>
    void applyFunction (BufferType& buffer, FunctionType&& function) noexcept;

    template <typename BufferType1, typename BufferType2, typename FunctionType>
    void applyFunction (const BufferType1& bufferSrc, BufferType2& bufferDest, FunctionType&& function) noexcept;

#if ! MARSDSP_NO_XSIMD

    template <typename BufferType, typename FunctionType, typename FloatType = BufferSampleType<BufferType>>
    std::enable_if_t<std::is_floating_point_v<FloatType>, void> applyFunctionSIMD (BufferType& buffer, FunctionType&& function) noexcept;

    template <typename BufferType1, typename BufferType2, typename FunctionType, typename FloatType = BufferSampleType<BufferType1>>
    std::enable_if_t<std::is_floating_point_v<FloatType>, void>
        applyFunctionSIMD (const BufferType1& bufferSrc, BufferType2& bufferDest, FunctionType&& function) noexcept;

    template <typename BufferType, typename SIMDFunctionType, typename ScalarFunctionType, typename FloatType = BufferSampleType<BufferType>>
    std::enable_if_t<std::is_floating_point_v<FloatType>, void>
        applyFunctionSIMD (BufferType& buffer, SIMDFunctionType&& simdFunction, ScalarFunctionType&& scalarFunction) noexcept;

    template <typename BufferType1, typename BufferType2, typename SIMDFunctionType, typename ScalarFunctionType, typename FloatType = BufferSampleType<BufferType1>>
    std::enable_if_t<std::is_floating_point_v<FloatType>, void>
        applyFunctionSIMD (const BufferType1& bufferSrc, BufferType2& bufferDest, SIMDFunctionType&& simdFunction, ScalarFunctionType&& scalarFunction) noexcept;
#endif
}
}
#include "buffer_math.cpp"
#endif
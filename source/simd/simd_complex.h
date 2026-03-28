#pragma once

#ifndef MEMORYBOY_SIMD_COMPLEX_H
#define MEMORYBOY_SIMD_COMPLEX_H

#include <complex>
#include <type_traits>
#include <JuceHeader.h>
#include <xsimd/xsimd.hpp>

//=================================================================================================================

#if ! MARSDSP_NO_XSIMD

#define MARSDSP_USING_XSIMD_STD(func) \
using std::func;                  \
using xsimd::func
#else
#define MARSDSP_USING_XSIMD_STD(func) \
using std::func
#endif

namespace MarsDSP::SIMD::
inline SIMDUtils {
#if ! MARSDSP_NO_XSIMD
        constexpr auto defaultSIMDAlignment = xsimd::default_arch::alignment();
#else
        constexpr size_t defaultSIMDAlignment = 16;
#endif

        template <typename T>
        bool all (const T& val)
        {
#if ! MARSDSP_NO_XSIMD
            if constexpr (! std::is_scalar_v<T>)
            {
                return xsimd::all (val);
            }
            else
#endif
            {
                return static_cast<bool> (val);
            }
        }
    }

//=================================================================================================================

namespace MarsDSP::SIMD::
inline ComplexMath {
    template <typename Type>
    xsimd::batch<Type> SIMDComplexMulReal (const xsimd::batch<std::complex<Type>>& a,
                                           const xsimd::batch<std::complex<Type>>& b)
    {
        return (a.real() * b.real()) - (a.imag() * b.imag());
    }

    template <typename Type>
    xsimd::batch<Type> SIMDComplexMulImag (const xsimd::batch<std::complex<Type>>& a,
                                           const xsimd::batch<std::complex<Type>>& b)
    {
        return (a.real() * b.imag()) + (a.imag() * b.real());
    }

    template <typename BaseType, typename OtherType>
    std::enable_if_t<std::is_same_v<dsp::SampleTypeHelpers::ElementType<OtherType>, BaseType>, xsimd::batch<std::complex<BaseType>>>
    pow (const xsimd::batch<std::complex<BaseType>>& a, OtherType x)
    {
        auto absa = xsimd::abs (a);
        auto arga = xsimd::arg (a);
        auto r = xsimd::pow (absa, xsimd::batch (x));
        auto theta = x * arga;
        auto sincosTheta = xsimd::sincos (theta);
        return { r * sincosTheta.second, r * sincosTheta.first };
    }

    template <typename BaseType, typename OtherType>
    std::enable_if_t<std::is_same_v<dsp::SampleTypeHelpers::ElementType<OtherType>, BaseType>, xsimd::batch<std::complex<BaseType>>>
        pow (OtherType a, const xsimd::batch<std::complex<BaseType>>& z)
    {

        const auto ze = xsimd::batch (static_cast<BaseType>(0));

        auto absa = xsimd::abs (a);
        auto arga = xsimd::select (a >= ze, ze, xsimd::batch (MathConstants<BaseType>::pi));
        auto x = z.real();
        auto y = z.imag();
        auto r = xsimd::pow (absa, x);
        auto theta = x * arga;

        auto cond = y == ze;
        r = select (cond, r, r * xsimd::exp (-y * arga));
        theta = select (cond, theta, theta + y * xsimd::log (absa));
        auto sincosTheta = xsimd::sincos (theta);
        return { r * sincosTheta.second, r * sincosTheta.first };
    }

    template <typename BaseType>
    xsimd::batch<std::complex<BaseType>> polar (const xsimd::batch<BaseType>& mag, const xsimd::batch<BaseType>& angle)
    {
        auto sincosTheta = xsimd::sincos (angle);
        return { mag * sincosTheta.second, mag * sincosTheta.first };
    }

    template <typename BaseType>
    static xsimd::batch<std::complex<BaseType>> polar (const xsimd::batch<BaseType>& angle)
    {
        auto sincosTheta = xsimd::sincos (angle);
        return { sincosTheta.second, sincosTheta.first };
    }

    //=================================================================================================================

    template <typename T>
    T hMaxSIMD (const xsimd::batch<T>& x)
    {
        constexpr auto vecSize = xsimd::batch<T>::size;
        T v alignas (xsimd::default_arch::alignment())[vecSize];
        xsimd::store_aligned (v, x);

        if constexpr (vecSize == 2)
            return juce::jmax (v[0], v[1]);
        else if constexpr (vecSize == 4)
            return juce::jmax (v[0], v[1], v[2], v[3]);
        else
            return juce::jmax (juce::jmax (v[0], v[1], v[2], v[3]), juce::jmax (v[4], v[5], v[6], v[7]));
    }

    template <typename T>
    T hMinSIMD (const xsimd::batch<T>& x)
    {
        constexpr auto vecSize = xsimd::batch<T>::size;
        T v alignas (xsimd::default_arch::alignment())[vecSize];
        xsimd::store_aligned (v, x);

        if constexpr (vecSize == 2)
            return juce::jmin (v[0], v[1]);
        else if constexpr (vecSize == 4)
            return juce::jmin (v[0], v[1], v[2], v[3]);
        else
            return juce::jmin (juce::jmin (v[0], v[1], v[2], v[3]), juce::jmin (v[4], v[5], v[6], v[7]));
    }

    template <typename T>
    T hAbsMaxSIMD (const xsimd::batch<T>& x)
    {
        return hMaxSIMD (xsimd::abs (x));
    }

    //=================================================================================================================
}
#endif
#pragma once

#ifndef MEMORYBOY_TYPE_UTILS_H
#define MEMORYBOY_TYPE_UTILS_H

#include <complex>
#include <xsimd/xsimd.hpp>

namespace MarsDSP::Utils::
inline SampleTypeHelpers
{
    /** Struct for determining a ample type element's type traits */
    template <typename T, bool = std::is_floating_point_v<T> || std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>>
    struct TypeTraits
    {
        using ElementType = T;
        static constexpr int Size = 1;
    };

#if ! MARSDSP_NO_XSIMD
    template <typename T>
    struct TypeTraits<xsimd::batch<T>, false>
    {
        using batch_type = xsimd::batch<T>;
        using ElementType = batch_type::value_type;
        static constexpr int Size = static_cast<int>(batch_type::size);
    };

    template <typename T>
    struct TypeTraits<const xsimd::batch<T>, false>
    {
        using batch_type = xsimd::batch<T>;
        using ElementType = const batch_type::value_type;
        static constexpr int Size = static_cast<int>(batch_type::size);
    };
#endif // ! MARSDSP_NO_XSIMD

    /** Type alias for a SIMD numeric type */
    template <typename SampleType>
    using NumericType = TypeTraits<SampleType>::ElementType;

    /** Type alias for retrieving a SIMD numeric type for a processor that has one defined */
    template <typename ProcessorType>
    using ProcessorNumericType = ProcessorType::NumericType;

#if ! MARSDSP_NO_XSIMD
    /** Useful template expression for determining if a type is a SIMDRegister */
    template <typename T, typename NumericType = NumericType<T>, typename SIMDType = xsimd::batch<NumericType>>
    inline constexpr bool IsSIMDRegister = std::is_same_v<T, SIMDType>;
#else
    /** Useful template expression for determining if a type is a SIMDRegister */
    template <typename>
    inline constexpr bool IsSIMDRegister = false;
#endif
}
#endif //MEMORYBOY_TYPE_UTILS_H
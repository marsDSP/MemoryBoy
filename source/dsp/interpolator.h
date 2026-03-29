#pragma once

#ifndef MEMORYBOY_INTERPOLATOR_H
#define MEMORYBOY_INTERPOLATOR_H

#include <JuceHeader.h>

#include "simd/simd_complex.h"
#include "utils/type_utils.h"

namespace MarsDSP::Buffers
{
    namespace DelayLineInterpolationTypes
    {
        struct None
        {
            template <typename T>
            void updateInternalVariables (int& , T& )
            {
            }

            template <typename SampleType, typename NumericType = Utils::SampleTypeHelpers::NumericType<SampleType>, typename StorageType = SampleType>
            SampleType call (const SampleType* buffer, int delayInt, NumericType  = {}, const SampleType&  = {})
            {
                return static_cast<SampleType> (buffer[delayInt]);
            }
        };

        struct Linear
        {
            template <typename T>
            void updateInternalVariables (int& , T& )
            {
            }

            template <typename SampleType, typename NumericType, typename StorageType = SampleType>
            SampleType call (const StorageType* buffer, int delayInt, NumericType delayFrac, const SampleType&  = {})
            {
                auto index1 = delayInt;
                auto index2 = index1 + 1;

                auto value1 = static_cast<SampleType> (buffer[index1]);
                auto value2 = static_cast<SampleType> (buffer[index2]);

                return value1 + static_cast<SampleType>(delayFrac) * (value2 - value1);
            }
        };

        struct Lagrange3rd
        {
            template <typename T>
            void updateInternalVariables (int& delayIntOffset, T& delayFrac)
            {
                if (delayIntOffset >= 1)
                {
                    ++delayFrac;
                    delayIntOffset--;
                }
            }

            template <typename SampleType, typename NumericType, typename StorageType = SampleType>
            SampleType call (const StorageType* buffer, int delayInt, NumericType delayFrac, const SampleType&  = {})
            {
                auto index1 = delayInt;
                auto index2 = index1 + 1;
                auto index3 = index2 + 1;
                auto index4 = index3 + 1;

                auto value1 = static_cast<SampleType> (buffer[index1]);
                auto value2 = static_cast<SampleType> (buffer[index2]);
                auto value3 = static_cast<SampleType> (buffer[index3]);
                auto value4 = static_cast<SampleType> (buffer[index4]);

                auto d1 = delayFrac - static_cast<NumericType>(1.0);
                auto d2 = delayFrac - static_cast<NumericType>(2.0);
                auto d3 = delayFrac - static_cast<NumericType>(3.0);

                auto c1 = -d1 * d2 * d3 / static_cast<NumericType>(6.0);
                auto c2 = d2 * d3 * static_cast<NumericType>(0.5);
                auto c3 = -d1 * d3 * static_cast<NumericType>(0.5);
                auto c4 = d1 * d2 / static_cast<NumericType>(6.0);

                return value1 * c1 + static_cast<SampleType>(delayFrac) * (value2 * c2 + value3 * c3 + value4 * c4);
            }
        };

        struct Lagrange5th
        {
            template <typename T>
            void updateInternalVariables (int& delayIntOffset, T& delayFrac)
            {
                if (delayIntOffset >= 2)
                {
                    delayFrac += static_cast<T>(2);
                    delayIntOffset -= 2;
                }
            }

            template <typename SampleType, typename NumericType, typename StorageType = SampleType>
            SampleType call (const StorageType* buffer, int delayInt, NumericType delayFrac, const SampleType&  = {})
            {
                auto index1 = delayInt;
                auto index2 = index1 + 1;
                auto index3 = index2 + 1;
                auto index4 = index3 + 1;
                auto index5 = index4 + 1;
                auto index6 = index5 + 1;

                auto value1 = static_cast<SampleType> (buffer[index1]);
                auto value2 = static_cast<SampleType> (buffer[index2]);
                auto value3 = static_cast<SampleType> (buffer[index3]);
                auto value4 = static_cast<SampleType> (buffer[index4]);
                auto value5 = static_cast<SampleType> (buffer[index5]);
                auto value6 = static_cast<SampleType> (buffer[index6]);

                auto d1 = delayFrac - static_cast<NumericType>(1.0);
                auto d2 = delayFrac - static_cast<NumericType>(2.0);
                auto d3 = delayFrac - static_cast<NumericType>(3.0);
                auto d4 = delayFrac - static_cast<NumericType>(4.0);
                auto d5 = delayFrac - static_cast<NumericType>(5.0);

                auto c1 = -d1 * d2 * d3 * d4 * d5 / static_cast<NumericType>(120.0);
                auto c2 = d2 * d3 * d4 * d5 / static_cast<NumericType>(24.0);
                auto c3 = -d1 * d3 * d4 * d5 / static_cast<NumericType>(12.0);
                auto c4 = d1 * d2 * d4 * d5 / static_cast<NumericType>(12.0);
                auto c5 = -d1 * d2 * d3 * d5 / static_cast<NumericType>(24.0);
                auto c6 = d1 * d2 * d3 * d4 / static_cast<NumericType>(120.0);

                return value1 * c1 + static_cast<SampleType>(delayFrac) * (value2 * c2 + value3 * c3 + value4 * c4 + value5 * c5 + value6 * c6);
            }
        };

        struct Thiran
        {
            template <typename T>
            void updateInternalVariables (int& delayIntOffset, T& delayFrac)
            {
                if (delayFrac < static_cast<T> (0.618) && delayIntOffset >= 1)
                {
                    ++delayFrac;
                    delayIntOffset--;
                }

                alpha = static_cast<double> ((1 - delayFrac) / (1 + delayFrac));
            }

            template <typename T1, typename T2>
            T1 call (const T1* buffer, int delayInt, T2 , T1& state)
            {
                auto index1 = delayInt;
                auto index2 = index1 + 1;

                auto value1 = buffer[index1];
                auto value2 = buffer[index2];

                auto output = value2 + static_cast<T1> (static_cast<T2> (alpha)) * (value1 - state);
                state = output;

                return output;
            }

            double alpha = 0.0;
        };

#ifndef DOXYGEN
        JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4324)
    #endif

        template <typename T, size_t N, size_t M = 256>
        struct Sinc
        {
            Sinc()
            {
                T cutoff = 0.455f;
                size_t j;
                for (j = 0; j < M + 1; j++)
                {
                    for (size_t i = 0; i < N; i++)
                    {
                        T t = -static_cast<T> (i) + static_cast<T> (N / static_cast<T> (2.0)) + static_cast<T> (j) / static_cast<T> (M) - static_cast<T> (1.0);
                        sinctable[j * N * 2 + i] = symmetric_blackman (t, static_cast<int> (N)) * cutoff * sincf (cutoff * t);
                    }
                }
                for (j = 0; j < M; j++)
                {
                    for (size_t i = 0; i < N; i++)
                        sinctable[j * N * 2 + N + i] = (sinctable[(j + 1) * N * 2 + i] - sinctable[j * N * 2 + i]) / static_cast<T> (65536.0);
                }
            }

            T sincf (T x) const noexcept
            {
                if (x == static_cast<T> (0))
                    return static_cast<T> (1);
                return (std::sin (MathConstants<T>::pi * x)) / (MathConstants<T>::pi * x);
            }

            T symmetric_blackman (T i, int n) const noexcept
            {
                i -= static_cast<T> (n / 2);
                return (static_cast<T> (0.42) - static_cast<T> (0.5) * std::cos (MathConstants<T>::twoPi * i / static_cast<T> (n))
                        + static_cast<T> (0.08) * std::cos (4 * MathConstants<T>::pi * i / static_cast<T> (n)));
            }

            template <typename SampleType, typename NumericType = SampleType, typename StorageType = SampleType>
            SampleType call (const StorageType* buffer, int delayInt, NumericType delayFrac, const SampleType&  = {})
            {
                const auto sincTableOffset = static_cast<size_t> ((static_cast<T> (1) - static_cast<T> (delayFrac)) * static_cast<T> (M)) * N * 2;

#if MARSDSP_NO_XSIMD

                auto out = static_cast<T> (0);
                for (size_t i = 0; i < N; ++i)
                    out += static_cast<T> (buffer[static_cast<size_t> (delayInt) + i]) * sinctable[sincTableOffset + i];
                return static_cast<SampleType> (out);

#else
                constexpr std::size_t batch_size = xsimd::batch<T>::size;
                const std::size_t aligned_N = N - (N % batch_size);

                auto out_simd = xsimd::batch<T> (static_cast<T> (0));
                for (size_t i = 0; i < aligned_N; i += batch_size)
                {
                    auto buff_reg = xsimd::load_unaligned (static_cast<const T *>(&buffer[static_cast<size_t>(delayInt) + i]));
                    auto sinc_reg = xsimd::load_aligned (&sinctable[sincTableOffset + i]);
                    out_simd += buff_reg * sinc_reg;
                }

                auto out = xsimd::reduce_add (out_simd);

                for (size_t i = aligned_N; i < N; ++i)
                    out += static_cast<T> (buffer[static_cast<size_t> (delayInt) + i]) * sinctable[sincTableOffset + i];

                return static_cast<SampleType> (out);
#endif
            }

            T sinctable alignas (SIMD::defaultSIMDAlignment)[(M + 1) * N * 2];
        };
        JUCE_END_IGNORE_WARNINGS_MSVC
}
}
#endif

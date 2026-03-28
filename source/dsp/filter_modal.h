#pragma once

#ifndef MEMORYBOY_FILTER_MODAL_H
#define MEMORYBOY_FILTER_MODAL_H

#include <JuceHeader.h>
#include "math/buffer_math.h"
#include "simd/simd_complex.h"

namespace MarsDSP::DSP::
inline Modal
{
    template<typename T>
    class ModalFilter
    {
    public:
        ModalFilter() = default;

        virtual ~ModalFilter() = default;

        virtual void prepare(T sampleRate);

        virtual void reset() noexcept { y1 = std::complex<T>{static_cast<T>(0.0)}; }
        virtual void setAmp(std::complex<T> amp) noexcept { amplitude = amp; }
        virtual void setAmp(T amp, T phase) noexcept { amplitude = std::polar(amp, phase); }

        virtual void setDecay(T newT60) noexcept
        {
            t60 = newT60;
            decayFactor = calcDecayFactor();
            updateParams();
        }

        virtual void setFreq(T newFreq) noexcept
        {
            freq = newFreq;
            oscCoef = calcOscCoef();
            updateParams();
        }

        [[nodiscard]] T getFreq() const noexcept { return freq; }

        virtual T processSample(T x)
        {
            auto y = filtCoef * y1 + amplitude * x;
            y1 = y;
            return std::imag(y);
        }

        virtual void processBlock(T *buffer, int numSamples);

    protected:
        void updateParams() noexcept { filtCoef = decayFactor * oscCoef; }

        T calcDecayFactor() noexcept
        {
            return std::pow(static_cast<T>(0.001), static_cast<T>(1) / (t60 * fs));
        }

        std::complex<T> calcOscCoef() noexcept
        {
            constexpr std::complex<T> jImag{0, 1};
            return std::exp(jImag * MathConstants<T>::twoPi * (freq / fs));
        }

        std::complex<T> filtCoef = 0;
        T decayFactor = 0;
        std::complex<T> oscCoef = 0;

        std::complex<T> y1 = 0;

        T freq = 1;
        T t60 = 1;
        std::complex<T> amplitude;
        T fs = 44100;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModalFilter)
    };

#if ! MARSDSP_NO_XSIMD

    template<typename FloatType>
    class ModalFilter<xsimd::batch<FloatType> >
    {
        using VType = xsimd::batch<FloatType>;
        using CType = xsimd::batch<std::complex<FloatType> >;

    public:
        ModalFilter() = default;

        virtual ~ModalFilter() = default;

        virtual void prepare(FloatType sampleRate);

        virtual void reset() noexcept { y1 = CType{}; }
        virtual void setAmp(VType amp) noexcept { amplitude = CType{amp, xsimd::batch(static_cast<FloatType>(0))}; }
        virtual void setAmp(CType amp) noexcept { amplitude = amp; }
        virtual void setAmp(VType amp, VType phase) noexcept { amplitude = polar(amp, phase); }

        virtual void setDecay(VType newT60) noexcept
        {
            t60 = newT60;
            decayFactor = calcDecayFactor();
            updateParams();
        }

        virtual void setFreq(VType newFreq) noexcept
        {
            freq = newFreq;
            oscCoef = calcOscCoef();
            updateParams();
        }

        [[nodiscard]] VType getFreq() const noexcept { return freq; }

        virtual VType processSample(VType x)
        {
            auto y = filtCoef * y1 + amplitude * x;
            y1 = y;
            return y.imag();
        }

        virtual void processBlock(VType *buffer, int numSamples);

    protected:
        void updateParams() noexcept { filtCoef = decayFactor * oscCoef; }

        VType calcDecayFactor() noexcept
        {
            using namespace SIMD::SIMDUtils;
            return xsimd::pow(static_cast<VType>(static_cast<FloatType>(0.001)), static_cast<VType>(1) / (t60 * fs));
        }

        CType calcOscCoef() noexcept
        {
            return polar((freq / fs) * MathConstants<FloatType>::twoPi);
        }

        CType filtCoef{};
        VType decayFactor = 0;
        CType oscCoef{};

        CType y1 = {};

        VType freq = 1;
        VType t60 = 1;
        CType amplitude;
        FloatType fs = 44100;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModalFilter)
    };
#endif
}
#endif

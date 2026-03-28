#pragma once

#ifndef MEMORYBOY_BRIGADE_DELAYLINE_H
#define MEMORYBOY_BRIGADE_DELAYLINE_H

#include <JuceHeader.h>
#include "brigade_filterbank.h"
#include "ou_process.h"
#include "math/buffer_math.h"

namespace MarsDSP::DSP
{
    template<size_t STAGES, bool ALIEN = false>
    class BucketBrigade
    {
    public:
        BucketBrigade() = default;
        BucketBrigade(BucketBrigade &&) noexcept = default;
        BucketBrigade &operator=(BucketBrigade &&) noexcept = default;

        void prepare(double sampleRate, juce::uint32 maximumBlockSize)
        {
            FS = static_cast<float>(sampleRate);
            Ts = 1.0f / FS;

            tn = 0.0f;
            evenOn = true;

            inputFilter = std::make_unique<InputFilterBank>(Ts);
            outputFilter = std::make_unique<OutputFilterBank>(Ts);
            H0 = outputFilter->calcH0();

            tapDrift.prepare(sampleRate, static_cast<int>(juce::jmax<juce::uint32>(maximumBlockSize, 1u)), 1);
            tapDrift.prepareBlock(0.0f, 1);
            modulationSampleIndex = 0;

            reset();
        }

        void reset()
        {
            bufferPtr = 0;
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            yBBD_old = 0.0f;
            tn = 0.0f;
            evenOn = true;
            modulationSampleIndex = 0;
            tapDrift.reset();
        }

        void setInputFilterFreq(float freqHz = BBDFilterSpec::inputFilterOriginalCutoff) const
        {
            inputFilter->set_freq(ALIEN ? freqHz * 0.2f : freqHz);
            inputFilter->set_time(tn);
        }

        void setOutputFilterFreq(float freqHz = BBDFilterSpec::outputFilterOriginalCutoff) const
        {
            outputFilter->set_freq(ALIEN ? freqHz * 0.2f : freqHz);
            outputFilter->set_time(tn);
        }

        void setDelayTime(float delaySec) noexcept
        {
            baseDelaySec = juce::jmax(Ts, delaySec - Ts);
            updateClockDelta(baseDelaySec);
        }

        void prepareModulationBlock(float amount, int numSamples)
        {
            modulationAmount = juce::jlimit(0.0f, 1.0f, amount);
            modulationSampleIndex = 0;
            tapDrift.prepareBlock(modulationAmount, juce::jmax(numSamples, 1));
        }

        template<bool A = ALIEN>
        std::enable_if_t<A, float>
        process(float u) noexcept
        {
            ScopedNoDenormals noDenormals;
            applyTapModulation();
            SIMDComplex<float> xOutAccum;
            float yBBD, delta;
            int iterations = 0;
            while (tn < 1.0f)
            {
                if (++iterations > 10000) break;
                if (evenOn)
                {
                    inputFilter->calcG();
                    buffer[bufferPtr++] = xsimd::reduce_add(
                        SIMD::SIMDComplexMulReal(inputFilter->Gcalc, inputFilter->x));
                    bufferPtr = (bufferPtr <= STAGES) ? bufferPtr : 0;
                } else
                {
                    yBBD = buffer[bufferPtr];
                    delta = yBBD - yBBD_old;
                    yBBD_old = yBBD;
                    outputFilter->calcG();
                    xOutAccum += outputFilter->Gcalc * delta;
                }

                evenOn = !evenOn;
                tn += Ts_bbd / Ts;
            }
            tn -= 1.0f;

            inputFilter->process(u);
            outputFilter->process(xOutAccum);
            float sumOut = xsimd::reduce_add(xOutAccum.real());
            return H0 * yBBD_old + sumOut;
        }

        template<bool A = ALIEN>
        std::enable_if_t<!A, float>
        process(float u) noexcept
        {
            ScopedNoDenormals noDenormals;
            applyTapModulation();
            SIMDComplex<float> xOutAccum{};
            float yBBD, delta;
            int iterations = 0;
            while (tn < Ts)
            {
                if (++iterations > 10000) break;
                if (evenOn)
                {
                    inputFilter->calcG();
                    buffer[bufferPtr++] = xsimd::reduce_add(
                        SIMD::SIMDComplexMulReal(inputFilter->Gcalc, inputFilter->x));
                    bufferPtr = (bufferPtr <= STAGES) ? bufferPtr : 0;
                } else
                {
                    yBBD = buffer[bufferPtr];
                    delta = yBBD - yBBD_old;
                    yBBD_old = yBBD;
                    outputFilter->calcG();
                    xOutAccum += outputFilter->Gcalc * delta;
                }

                evenOn = !evenOn;
                tn += Ts_bbd;
            }
            tn -= Ts;

            inputFilter->process(u);
            outputFilter->process(xOutAccum);
            float sumOut = xsimd::reduce_add(xOutAccum.real());
            return H0 * yBBD_old + sumOut;
        }

    private:
        void updateClockDelta(float delaySec) noexcept
        {
            const auto clockRateHz = (2.0f * static_cast<float>(STAGES)) / juce::jmax(delaySec, Ts);
            Ts_bbd = 1.0f / clockRateHz;
            Ts_bbd = juce::jmax(Ts * 0.01f, Ts_bbd);

            const auto doubleTs = 2.0f * Ts_bbd;
            inputFilter->set_delta(doubleTs);
            outputFilter->set_delta(doubleTs);
        }

        void applyTapModulation() noexcept
        {
            if (modulationAmount <= 0.0f)
                return;

            constexpr auto maxDelayDrift = 0.12f;

            const auto drift = tapDrift.process(modulationSampleIndex++, 0);
            const auto driftRatio = juce::jlimit(0.8f,
                                                 1.2f,
                                                 1.0f + drift * maxDelayDrift);
            updateClockDelta(baseDelaySec * driftRatio);
        }

        float FS = 48000.0f;
        float Ts = 1.0f / FS;
        float Ts_bbd = Ts;
        float baseDelaySec = Ts;
        float modulationAmount = 0.0f;
        int modulationSampleIndex = 0;

        std::unique_ptr<InputFilterBank> inputFilter;
        std::unique_ptr<OutputFilterBank> outputFilter;
        float H0 = 1.0f;
        OUProcess tapDrift;

        std::array<float, STAGES + 1> buffer;
        size_t bufferPtr = 0;

        float yBBD_old = 0.0f;
        float tn = 0.0f;
        bool evenOn = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BucketBrigade)
    };
}
#endif

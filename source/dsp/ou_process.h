#pragma once

#ifndef MEMORYBOY_OU_PROCESS_H
#define MEMORYBOY_OU_PROCESS_H

#include <JuceHeader.h>
#include "noise.h"

namespace MarsDSP::DSP
{
    class OUProcess
    {
    public:
        OUProcess() = default;

        void prepare(double sampleRate, int samplesPerBlock, int numChannels)
        {
            juce::dsp::ProcessSpec spec
            {
                sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(numChannels)
            };

            juce::dsp::ProcessSpec monoSpec
            {
                sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1
            };

            noiseGen.setNoiseType(noise<float>::Uniform);
            noiseGen.setGainLinear(1.0f / 2.33f);
            noiseGen.prepare(monoSpec);

            lpf.prepare(spec);
            lpf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            lpf.setCutoffFrequency(2.5f);
            lpf.reset();

            noiseBuffer.setSize(1, samplesPerBlock);
            rPtr = noiseBuffer.getReadPointer(0);

            sqrtdelta = 1.0f / std::sqrt(static_cast<float>(sampleRate));
            T = 1.0f / static_cast<float>(sampleRate);

            y.assign(static_cast<size_t>(numChannels), 0.0f);
            reset();
        }

        void reset()
        {
            noiseGen.reset();
            lpf.reset();
            std::fill(y.begin(), y.end(), 0.0f);
        }

        void prepareBlock(float amtParam, int numSamples)
        {
            noiseBuffer.setSize(1, numSamples, false, false, true);
            noiseBuffer.clear();
            rPtr = noiseBuffer.getReadPointer(0);

            juce::dsp::AudioBlock<float> noiseBlock(noiseBuffer);
            noiseGen.process(juce::dsp::ProcessContextReplacing<float>(noiseBlock));

            amt = juce::jlimit(0.0f, 1.0f, amtParam);
            damping = 0.5f + amt * 8.0f;
            mean = 0.0f;
        }

        void setMeanReversion(float newMeanReversion) noexcept
        {
            meanReversion = juce::jlimit(0.0f, 1.0f, newMeanReversion);
        }

        float process(int n, size_t ch) noexcept
        {
            if (amt <= 0.0f || rPtr == nullptr)
                return 0.0f;

            y[ch] += sqrtdelta * rPtr[n] * amt;
            const auto driftToMean = juce::jmap(meanReversion, 0.5f, 8.5f);
            y[ch] += damping * driftToMean * (mean - y[ch]) * T;
            return lpf.processSample(static_cast<int>(ch), y[ch]);
        }

    private:
        float sqrtdelta = 1.0f / std::sqrt(48000.0f);
        float T = 1.0f / 48000.0f;
        std::vector<float> y;

        float amt = 0.0f;
        float mean = 0.0f;
        float damping = 0.0f;
        float meanReversion = 0.0f;

        noise<float> noiseGen;
        juce::AudioBuffer<float> noiseBuffer;
        const float *rPtr = nullptr;

        juce::dsp::StateVariableTPTFilter<float> lpf;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OUProcess)
    };
}
#endif
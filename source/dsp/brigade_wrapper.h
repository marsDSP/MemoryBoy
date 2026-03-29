#pragma once

#ifndef MEMORYBOY_BRIGADE_WRAPPER_H
#define MEMORYBOY_BRIGADE_WRAPPER_H

#include <JuceHeader.h>
#include "base_delayline.h"
#include "brigade_delayline.h"
#include "diffusor.h"

namespace MarsDSP::DSP
{
    template<size_t STAGES, bool ALIEN = false>
    class BBDWrapper : public DelayLineBase<float>
    {
        using StereoDiffuser = DiffuserChain<4, Diffuser<float, 2>>;

    public:
        BBDWrapper() = default;

        void setFilterFreq(float freqHz)
        {
            setInputFilterFreq(freqHz);
            setOutputFilterFreq(freqHz);
        }

        void setInputFilterFreq(float freqHz)
        {
            for (auto &line: lines)
                line.setInputFilterFreq(freqHz);
        }

        void setOutputFilterFreq(float freqHz)
        {
            for (auto &line: lines)
                line.setOutputFilterFreq(freqHz);
        }

        void setDelay(float newDelayInSamples) final
        {
            delaySamp = newDelayInSamples;
            auto delaySec = delaySamp / sampleRate;
            for (auto &line: lines)
                line.setDelayTime(delaySec);
        }

        [[nodiscard]] float getDelay() const final
        {
            return delaySamp;
        }

        void setTapModulation(float newAmount)
        {
            tapModulationAmount = jlimit(0.0f, 1.0f, newAmount);
        }

        void setDiffusor(float newAmount)
        {
            diffusorAmount = jlimit(0.0f, 1.0f, newAmount);
            updateDiffuserSettings();
        }

        void prepareModulationBlock(int numSamples)
        {
            for (auto &line: lines)
                line.prepareModulationBlock(tapModulationAmount, numSamples);
        }

        void prepare(const dsp::ProcessSpec &spec) final
        {
            sampleRate = static_cast<float>(spec.sampleRate);
            inputs.resize(spec.numChannels, 0.0f);
            processedInputs.resize(spec.numChannels, 0.0f);
            outputs.resize(spec.numChannels, 0.0f);

            lines.clear();
            for (size_t ch = 0; ch < spec.numChannels; ++ch)
            {
                lines.emplace_back();
                lines[ch].prepare(sampleRate, spec.maximumBlockSize);
                lines[ch].setInputFilterFreq();
                lines[ch].setOutputFilterFreq();
            }

            stereoDiffuserPrepared = spec.numChannels >= 2;
            if (stereoDiffuserPrepared)
            {
                if (stereoDiffuser == nullptr)
                    stereoDiffuser = std::make_unique<StereoDiffuser>();

                stereoDiffuser->prepare<DiffuserChainEqualConfig>(spec.sampleRate);
                stereoDiffuser->reset();
                updateDiffuserSettings();
            }
            else
            {
                stereoDiffuser.reset();
            }
        }

        void free() final
        {
            inputs.clear();
            processedInputs.clear();
            outputs.clear();
            lines.clear();
            stereoDiffuser.reset();
            stereoDiffuserPrepared = false;
        }

        void reset() final
        {
            for (auto &line: lines)
                line.reset();

            if (stereoDiffuser != nullptr)
                stereoDiffuser->reset();
        }

        const std::vector<float>& processFrame(const float* frameData, int numChannels) noexcept
        {
            const auto channelsToProcess = jmin(numChannels, static_cast<int>(lines.size()));
            jassert(channelsToProcess >= 0);

            if (channelsToProcess >= 2 && diffusorAmount > 0.0f && stereoDiffuserPrepared)
            {
                stereoFrame[0] = frameData[0];
                stereoFrame[1] = frameData[1];

                const auto* diffusedFrame = stereoDiffuser->process(stereoFrame.data());
                processedInputs[0] = jmap(diffusorAmount, frameData[0], diffusedFrame[0]);
                processedInputs[1] = jmap(diffusorAmount, frameData[1], diffusedFrame[1]);
            }

            for (int channel = 0; channel < channelsToProcess; ++channel)
            {
                if (!(channelsToProcess >= 2 && channel < 2 && diffusorAmount > 0.0f && stereoDiffuserPrepared))
                    processedInputs[static_cast<size_t>(channel)] = frameData[channel];

                outputs[static_cast<size_t>(channel)] = lines[static_cast<size_t>(channel)].process(processedInputs[static_cast<size_t>(channel)]);
            }

            return outputs;
        }

        void pushSample(int channel, float sample) noexcept final
        {
            inputs[static_cast<size_t>(channel)] = sample;
        }

        float popSample(int channel) noexcept final
        {
            return lines[static_cast<size_t>(channel)].process(inputs[static_cast<size_t>(channel)]);
        }

    private:
        void updateDiffuserSettings()
        {
            if (!stereoDiffuserPrepared || stereoDiffuser == nullptr)
                return;

            constexpr auto maxDiffusionTimeMs = 40.0f;
            stereoDiffuser->setDiffusionTimeMs(diffusorAmount * maxDiffusionTimeMs);
        }

        float popSample(int, float, bool) noexcept final
        {
            return 0.0f;
        }

        void incrementReadPointer(int) noexcept final
        {
        }

        float delaySamp = 1.0f;
        float sampleRate = 48000.0f;
        float tapModulationAmount = 0.0f;
        float diffusorAmount = 0.0f;
        bool stereoDiffuserPrepared = false;

        std::deque<BucketBrigade<STAGES, ALIEN> > lines;
        std::vector<float> inputs;
        std::vector<float> processedInputs;
        std::vector<float> outputs;
        std::unique_ptr<StereoDiffuser> stereoDiffuser;
        std::array<float, 2> stereoFrame{};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BBDWrapper)
    };
}
#endif

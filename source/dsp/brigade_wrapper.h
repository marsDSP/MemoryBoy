#pragma once

#ifndef MEMORYBOY_BRIGADE_WRAPPER_H
#define MEMORYBOY_BRIGADE_WRAPPER_H

#include <JuceHeader.h>
#include "base_delayline.h"
#include "brigade_delayline.h"

namespace MarsDSP::DSP
{
    template<size_t STAGES, bool ALIEN = false>
    class BBDWrapper : public DelayLineBase<float>
    {
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

        [[nodiscard]] float getDelay() const final { return delaySamp; }

        void prepare(const juce::dsp::ProcessSpec &spec) final
        {
            sampleRate = static_cast<float>(spec.sampleRate);
            inputs.resize(spec.numChannels, 0.0f);

            lines.clear();
            for (size_t ch = 0; ch < spec.numChannels; ++ch)
            {
                lines.emplace_back();
                lines[ch].prepare(sampleRate);
                lines[ch].setInputFilterFreq();
                lines[ch].setOutputFilterFreq();
            }
        }

        void free() final
        {
            inputs.clear();
            lines.clear();
        }

        void reset() final
        {
            for (auto &line: lines)
                line.reset();
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
        float popSample(int, float, bool) noexcept final
        {
            return 0.0f;
        }

        void incrementReadPointer(int) noexcept final
        {
        }

        float delaySamp = 1.0f;
        float sampleRate = 48000.0f;

        std::vector<BucketBrigade<STAGES, ALIEN> > lines;
        std::vector<float> inputs;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BBDWrapper)
    };
}
#endif

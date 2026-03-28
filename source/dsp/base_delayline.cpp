#include "base_delayline.h"

namespace MarsDSP::DSP
{
    //==============================================================================
    template<typename SampleType, typename InterpolationType, typename StorageType>
    DelayLine<SampleType, InterpolationType, StorageType>::DelayLine()
        : DelayLine(1 << 18)
    {
    }

    template<typename SampleType, typename InterpolationType, typename StorageType>
    DelayLine<SampleType, InterpolationType, StorageType>::DelayLine(int maximumDelayInSamples)
    {
        jassert(maximumDelayInSamples >= 0);

        totalSize = juce::jmax(4, maximumDelayInSamples + 1);
    }

    //==============================================================================
    template<typename SampleType, typename InterpolationType, typename StorageType>
    void DelayLine<SampleType, InterpolationType, StorageType>::setDelay(
        DelayLine<SampleType, InterpolationType, StorageType>::NumericType newDelayInSamples)
    {
        auto upperLimit = static_cast<NumericType>(totalSize - 1);
        jassert(juce::isPositiveAndNotGreaterThan (newDelayInSamples, upperLimit));

        delay = juce::jlimit(static_cast<NumericType>(0), upperLimit, newDelayInSamples);
        delayInt = static_cast<int>(std::floor(delay));
        delayFrac = delay - static_cast<NumericType>(delayInt);

        interpolator.updateInternalVariables(delayInt, delayFrac);
    }

    template<typename SampleType, typename InterpolationType, typename StorageType>
    Utils::ProcessorNumericType<DelayLine<SampleType, InterpolationType, StorageType> >
    DelayLine<SampleType, InterpolationType, StorageType>::getDelay() const
    {
        return delay;
    }

    //==============================================================================
    template<typename SampleType, typename InterpolationType, typename StorageType>
    void DelayLine<SampleType, InterpolationType, StorageType>::prepare(const juce::dsp::ProcessSpec &spec)
    {
        jassert(spec.numChannels > 0);

        this->bufferData.setMaxSize(static_cast<int>(spec.numChannels), 2 * totalSize);

        this->writePos.resize(spec.numChannels);
        this->readPos.resize(spec.numChannels);

        this->v.resize(spec.numChannels);

        reset();

        bufferPtrs.resize(spec.numChannels);
        for (int ch = 0; ch < static_cast<int>(spec.numChannels); ++ch)
            bufferPtrs[static_cast<size_t>(ch)] = this->bufferData.getWritePointer(ch);
    }

    template<typename SampleType, typename InterpolationType, typename StorageType>
    void DelayLine<SampleType, InterpolationType, StorageType>::free()
    {
        this->bufferData.setMaxSize(0, 0);

        this->writePos.clear();
        this->readPos.clear();
        this->v.clear();
        bufferPtrs.clear();
    }

    template<typename SampleType, typename InterpolationType, typename StorageType>
    void DelayLine<SampleType, InterpolationType, StorageType>::reset()
    {
        for (auto vec: {&this->writePos, &this->readPos})
            std::fill(vec->begin(), vec->end(), 0);

        std::fill(this->v.begin(), this->v.end(), static_cast<SampleType>(0));

        this->bufferData.clear();
    }
}

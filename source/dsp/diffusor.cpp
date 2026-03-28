#include "diffusor.h"

namespace MarsDSP::DSP
{
    inline double DefaultDiffuserConfig::getDelayMult (int channelIndex, int nChannels, std::mt19937& mt)
{
    const auto rangeLow = static_cast<double>(channelIndex + 1) / static_cast<double>(nChannels + 1);
    const auto rangeHigh = static_cast<double>(channelIndex + 2) / static_cast<double>(nChannels + 1);

    auto&& dist = std::uniform_real_distribution<> (rangeLow, rangeHigh);
    return dist (mt);
}

inline double DefaultDiffuserConfig::getPolarityMultiplier (int /*channelIndex*/, int /*nChannels*/, std::mt19937& mt)
{
    auto&& dist = std::uniform_int_distribution<> (0, 1);
    return dist (mt) == 0 ? 1.0 : -1.0;
}

inline void DefaultDiffuserConfig::fillChannelSwapIndexes (size_t* indexes, int numChannels, std::mt19937& mt)
{
    std::iota (indexes, indexes + numChannels, 0);
    std::shuffle (indexes, indexes + numChannels, mt);
}

//======================================================================
template <typename FloatType, int nChannels, typename DelayInterpType, int delayBufferSize, typename StorageType>
template <typename DiffuserConfig>
void Diffuser<FloatType, nChannels, DelayInterpType, delayBufferSize, StorageType>::prepare (double sampleRate)
{
    fsOver1000 = static_cast<FloatType>(sampleRate) / static_cast<FloatType>(1000);

    std::random_device rd;
    std::mt19937 randGenerator (rd());

    DiffuserConfig::fillChannelSwapIndexes (channelSwapIndexes.data(), nChannels, randGenerator);

    for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
    {
        delays[i].reset();
        delayWritePointer = 0;

        delayRelativeMults[i] = static_cast<FloatType>(DiffuserConfig::getDelayMult(static_cast<int>(i), nChannels, randGenerator));
        polarityMultipliers[i] = static_cast<FloatType>(DiffuserConfig::getPolarityMultiplier(static_cast<int>(i), nChannels, randGenerator));
    }
}

template <typename FloatType, int nChannels, typename DelayInterpType, int delayBufferSize, typename StorageType>
void Diffuser<FloatType, nChannels, DelayInterpType, delayBufferSize, StorageType>::reset()
{
    for (auto& delay : delays)
        delay.reset();
}

template <typename FloatType, int nChannels, typename DelayInterpType, int delayBufferSize, typename StorageType>
void Diffuser<FloatType, nChannels, DelayInterpType, delayBufferSize, StorageType>::setDiffusionTimeMs (FloatType diffusionTimeMs)
{
    for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
    {
        const auto delayTimesSamples = delayRelativeMults[i] * diffusionTimeMs * fsOver1000;
        delayReadPointers[i] = DelayType::getReadPointer (delayWritePointer, delayTimesSamples);
    }
}

//======================================================================
template <int nStages, typename DiffuserType>
template <typename DiffuserChainConfig, typename DiffuserConfig>
void DiffuserChain<nStages, DiffuserType>::prepare (double sampleRate)
{
    for (size_t i = 0; i < static_cast<size_t>(nStages); ++i)
    {
        stages[i].template prepare<DiffuserConfig> (sampleRate);
        diffusionTimeMults[i] = static_cast<FloatType>(DiffuserChainConfig::getDiffusionMult(static_cast<int>(i), nStages));
    }
}

template <int nStages, typename DiffuserType>
void DiffuserChain<nStages, DiffuserType>::reset()
{
    for (auto& stage : stages)
        stage.reset();
}

template <int nStages, typename DiffuserType>
void DiffuserChain<nStages, DiffuserType>::setDiffusionTimeMs (FloatType diffusionTimeMs)
{
    for (size_t i = 0; i < static_cast<size_t>(nStages); ++i)
        stages[i].setDiffusionTimeMs (diffusionTimeMs * diffusionTimeMults[i]);
}
}
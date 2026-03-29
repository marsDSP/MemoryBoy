#pragma once

#ifndef MEMORYBOY_DIFFUSOR_H
#define MEMORYBOY_DIFFUSOR_H
#include <array>
#include <cmath>
#include <random>
#include <JuceHeader.h>

#include "math/buffer_staticdelay.h"
#include "math/matrix_ops.h"

namespace MarsDSP::DSP
{
    /**
 * Default configuration for a diffuser.
 * To use a custom configuration, make a derived class from this one,
 * and override some methods.
 */
struct DefaultDiffuserConfig
{
    /** Chooses random delay multipliers in equally spaced regions */
    static double getDelayMult (int channelIndex, int nChannels, std::mt19937& mt);

    /** Chooses polarity multipliers randomly */
    static double getPolarityMultiplier (int channelIndex, int nChannels, std::mt19937& mt);

    static void fillChannelSwapIndexes (size_t* indexes, int numChannels, std::mt19937& mt);
};

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4324) // structure was padded due to alignment specifier

/**
 * Simple diffuser configuration with:
 *   - Delay diffusion
 *   - Polarity flipping
 *   - Hadamard mixing
 */
template <typename FloatType,
          int nChannels,
          typename DelayInterpType = Buffers::DelayLineInterpolationTypes::None,
          int delayBufferSize = 1 << 18,
          typename StorageType = FloatType>
class Diffuser
{
    using DelayType = Buffers::StaticDelayBuffer<FloatType, DelayInterpType, delayBufferSize, StorageType>;

public:
    using Float = FloatType;

    Diffuser() = default;

    /** Prepares the diffuser for a given sample rate and configuration */
    template <typename DiffuserConfig = DefaultDiffuserConfig>
    void prepare (double sampleRate)
    {
        fsOver1000 = static_cast<FloatType>(sampleRate) / static_cast<FloatType>(1000);

        std::random_device rd;
        std::mt19937 randGenerator (rd());

        DiffuserConfig::fillChannelSwapIndexes (channelSwapIndexes.data(), nChannels, randGenerator);

        delayWritePointer = 0;
        for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
        {
            delays[i].reset();
            delayRelativeMults[i] = static_cast<FloatType> (DiffuserConfig::getDelayMult (static_cast<int> (i), nChannels, randGenerator));
            polarityMultipliers[i] = static_cast<FloatType> (DiffuserConfig::getPolarityMultiplier (static_cast<int> (i), nChannels, randGenerator));
            delayReadPointers[i] = FloatType {};
        }
    }

    /** Resets the diffuser state */
    void reset()
    {
        delayWritePointer = 0;
        for (auto& delay : delays)
            delay.reset();

        std::fill (delayReadPointers.begin(), delayReadPointers.end(), FloatType {});
        std::fill (outData.begin(), outData.end(), FloatType {});
    }

    /** Sets the diffusion time in milliseconds */
    void setDiffusionTimeMs (FloatType diffusionTimeMs)
    {
        for (size_t i = 0; i < static_cast<size_t> (nChannels); ++i)
        {
            const auto delayTimesSamples = delayRelativeMults[i] * diffusionTimeMs * fsOver1000;
            delayReadPointers[i] = DelayType::getReadPointer (delayWritePointer, delayTimesSamples);
        }
    }

    /** Processes a set of channels */
    inline const FloatType* process (const FloatType* data) noexcept
    {
        using HadamardMix = Math::MatrixOps::Hadamard<FloatType, nChannels>;

        // Delay
        for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
            delays[i].pushSample (data[i], delayWritePointer);
        DelayType::decrementPointer (delayWritePointer);

        for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
        {
            outData[i] = delays[channelSwapIndexes[i]].popSample (delayReadPointers[i]);
            DelayType::decrementPointer (delayReadPointers[i]);
        }

        // Mix with a Hadamard matrix
        if constexpr (std::is_floating_point_v<FloatType>)
        {
            if (Math::SIMDUtils::isAligned (outData.data()))
            {
                HadamardMix::inPlace (outData.data());
            }
            else
            {
                HadamardMix::recursiveUnscaled (outData.data(), outData.data());

                constexpr auto scalingFactor = static_cast<FloatType> (gcem::sqrt (1.0 / static_cast<double> (nChannels)));
                for (auto& sample : outData)
                    sample *= scalingFactor;
            }
        }
        else
        {
            HadamardMix::inPlace (outData.data());
        }

        // Flip some polarities
        for (size_t i = 0; i < static_cast<size_t>(nChannels); ++i)
            outData[i] *= polarityMultipliers[i];

        return outData.data();
    }

private:
    std::array<DelayType, static_cast<size_t>(nChannels)> delays;
    std::array<FloatType, static_cast<size_t>(nChannels)> delayRelativeMults;
    std::array<FloatType, static_cast<size_t>(nChannels)> polarityMultipliers;
    std::array<size_t, static_cast<size_t>(nChannels)> channelSwapIndexes;

    int delayWritePointer = 0;
    std::array<FloatType, static_cast<size_t>(nChannels)> delayReadPointers;

#if MarsDSP_REVERB_ALIGN_IO
    alignas (SIMDUtils::defaultSIMDAlignment) std::array<FloatType, (size_t) nChannels> outData;
#else
    std::array<FloatType, static_cast<size_t>(nChannels)> outData;
#endif

    FloatType fsOver1000 = FloatType (48000 / 1000);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Diffuser)
};

/** Configuration of diffusers with equal diffusion times */
struct DiffuserChainEqualConfig
{
    static double getDiffusionMult (int /*stageIndex*/, int /*nStages*/) { return 1.0; }
};

/** Configuration of diffusers where each diffuser is half the length of the next one */
struct DiffuserChainHalfConfig
{
    static double getDiffusionMult (int stageIndex, int nStages)
    {
        return std::pow (2.0, static_cast<double>(stageIndex) - static_cast<double>(nStages) / 2.0 - 0.5);
    }
};

/** A chain of diffusers */
template <int nStages, typename DiffuserType = Diffuser<float, 8>>
class DiffuserChain
{
    using FloatType = DiffuserType::Float;

public:
    DiffuserChain() = default;

    /** Prepares the diffuser chain with a given sample rate and configurations */
    template <typename DiffuserChainConfig = DiffuserChainEqualConfig, typename DiffuserConfig = DefaultDiffuserConfig>
    void prepare (double sampleRate)
    {
        for (size_t i = 0; i < static_cast<size_t> (nStages); ++i)
        {
            stages[i].template prepare<DiffuserConfig> (sampleRate);
            diffusionTimeMults[i] = static_cast<FloatType> (DiffuserChainConfig::getDiffusionMult (static_cast<int> (i), nStages));
        }
    }

    /** Resets the state of each diffuser */
    void reset()
    {
        for (auto& stage : stages)
            stage.reset();
    }

    /** Sets the diffusion time of the diffusion chain */
    void setDiffusionTimeMs (FloatType diffusionTimeMs)
    {
        for (size_t i = 0; i < static_cast<size_t> (nStages); ++i)
            stages[i].setDiffusionTimeMs (diffusionTimeMs * diffusionTimeMults[i]);
    }

    /** Processes a set of channels */
    const FloatType* process (const FloatType* data) noexcept
    {
        const FloatType* outData = data;
        for (auto& stage : stages)
            outData = stage.process (outData);

        return outData;
    }

private:
    std::array<DiffuserType, static_cast<size_t>(nStages)> stages;
    std::array<FloatType, static_cast<size_t>(nStages)> diffusionTimeMults;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiffuserChain)
};

JUCE_END_IGNORE_WARNINGS_MSVC
}

#endif
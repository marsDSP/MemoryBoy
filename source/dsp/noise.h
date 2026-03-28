#pragma once

#ifndef MEMORYBOY_NOISE_H
#define MEMORYBOY_NOISE_H

#include <JuceHeader.h>
#include "utils/type_utils.h"

namespace MarsDSP::DSP
{
#ifndef DOXYGEN
    namespace NoiseHelpers
    {
        template <typename T>
        T uniform01 (juce::Random&) noexcept;
    }
#endif

    template <typename T>
    class noise : public juce::dsp::Gain<Utils::NumericType<T>>
    {
        using NumericType = Utils::NumericType<T>;

    public:
        enum NoiseType
        {
            Uniform, /**< Uniform white noise [-1, 1] */
            Normal, /**< White noise with a normal/Gaussian distribution, generated using the Box-Muller Transform */
            Pink, /**< Pink noise (-3dB / Oct), generated using the Voss algorithm */
        };

        noise() = default;

        /** Selects a new noise profile */
        void setNoiseType (NoiseType newType) noexcept { type = newType; }

        /** Returns the current noise profile */
        [[nodiscard]] NoiseType getNoiseType() const noexcept { return type; }

        /** Called before processing starts. */
        void prepare (const juce::dsp::ProcessSpec& spec) noexcept;

        /** Resets the internal state of the gain */
        void reset() noexcept;

        /** Sets the seed for the random number generator */
        void setSeed (juce::int64 newSeed) { rand.setSeed (newSeed); }

        /** Processes the input and output buffers supplied in the processing context. */
        template <typename ProcessContext>
        void process (const ProcessContext& context) noexcept;

        /** Adds noise to a buffer of audio data. */
        void processBlock (juce::dsp::AudioBlock<T>& block) noexcept;

    private:
        T processSample (T) { return static_cast<T>(0); } // hide from dsp::Gain

        inline void applyGain (dsp::AudioBlock<T>& block)
        {
            dsp::ProcessContextReplacing<T> context (block);
            juce::dsp::Gain<NumericType>::process (context);
        }

        NoiseType type;
        juce::Random rand;

        /** Based on "The Voss algorithm"
            http://www.firstpr.com.au/dsp/pink-noise/
        */
        template <size_t QUALITY = 8>
        struct PinkNoiseGenerator
        {
            std::vector<int> frame;
            std::vector<std::array<T, QUALITY>> values;

            void reset (size_t nChannels)
            {
                frame.resize (nChannels, -1);

                values.clear();
                for (size_t ch = 0; ch < nChannels; ++ch)
                {
                    std::array<T, QUALITY> v;
                    v.fill (static_cast<T>(0));
                    values.push_back (v);
                }
            }

            /** Generates pink noise (-3dB / octave) */
            inline T operator() (size_t ch, juce::Random& r) noexcept
            {
                int lastFrame = frame[ch];
                frame[ch]++;
                if (frame[ch] >= (1 << QUALITY))
                    frame[ch] = 0;
                int diff = lastFrame ^ frame[ch];

                auto sum = static_cast<T>(0);
                for (size_t i = 0; i < QUALITY; i++)
                {
                    if (diff & (1 << i))
                    {
                        values[ch][i] = NoiseHelpers::uniform01<T> (r) - static_cast<T>(0.5);
                    }
                    sum += values[ch][i];
                }

                return sum * oneOverEight;
            }

            const T oneOverEight = static_cast<T> (1.0 / 8.0);
        };

        PinkNoiseGenerator<> pink;

        juce::HeapBlock<char> randBlockData;
        dsp::AudioBlock<T> randBlock;

        juce::HeapBlock<char> gainBlockData;
        dsp::AudioBlock<T> gainBlock;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (noise)
    };

#endif
#include "noise.h"

    template <typename T>
    void noise<T>::prepare (const juce::dsp::ProcessSpec& spec) noexcept
    {
        juce::dsp::Gain<NumericType>::prepare (spec);

        randBlockData.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (T), true);
        randBlock = dsp::AudioBlock<T> (randBlockData, spec.numChannels, spec.maximumBlockSize);
        randBlock.clear();

        gainBlockData.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (T), true);
        gainBlock = dsp::AudioBlock<T> (gainBlockData, spec.numChannels, spec.maximumBlockSize);
        gainBlock.clear();

        pink.reset (spec.numChannels);
    }

    template <typename T>
    void noise<T>::reset() noexcept
    {
        juce::dsp::Gain<NumericType>::reset();
    }

#ifndef DOXYGEN
    namespace NoiseHelpers
    {
        /** Returns a uniform random number in [0, 1) */
        template <>
        inline double uniform01 (juce::Random& r) noexcept
        {
            return r.nextDouble();
        }

        /** Returns a uniform random number in [0, 1) */
        template <>
        inline float uniform01 (juce::Random& r) noexcept
        {
            return r.nextFloat();
        }

        /** Generates white noise with a uniform distribution */
        template <typename T>
        struct uniformCentered
        {
            inline T operator() (size_t /*ch*/, juce::Random& r) const noexcept
            {
                return static_cast<T>(2) * uniform01<T> (r) - static_cast<T>(1);
            }
        };

        /** Generates white noise with a normal (Gaussian) distribution */
        template <typename T>
        struct normal
        {
            inline T operator() (size_t /*ch*/, juce::Random& r) const noexcept
            {
                // Box-Muller transform
                T radius = std::sqrt (static_cast<T>(-2) * std::log (static_cast<T>(1) - uniform01<T> (r)));
                T theta = juce::MathConstants<T>::twoPi * uniform01<T> (r);
                T value = radius * std::sin (theta) / juce::MathConstants<T>::sqrt2;
                return value;
            }
        };

        /** Process audio context with some random functor */
        template <typename T, typename ProcessContext, typename F>
        void processRandom (const ProcessContext& context, juce::Random& r, F randFunc) noexcept
        {
            auto&& outBlock = context.getOutputBlock();

            auto len = outBlock.getNumSamples();
            auto numChannels = outBlock.getNumChannels();

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto* dst = outBlock.getChannelPointer (ch);

                for (size_t i = 0; i < len; ++i)
                    dst[i] = randFunc (ch, r);
            }
        }

    }
#endif

    template <typename T>
    template <typename ProcessContext>
    void noise<T>::process (const ProcessContext& context) noexcept
    {
        // bypass
        if (context.isBypassed)
            return;

        auto&& outBlock = context.getOutputBlock();
        auto&& inBlock = context.getInputBlock();
        auto len = outBlock.getNumSamples();

        auto randSubBlock = randBlock.getSubBlock (0, len);
        juce::dsp::ProcessContextReplacing<T> randContext (randSubBlock);

        // generate random block
        if (type == Uniform)
            NoiseHelpers::processRandom<T> (randContext, rand, NoiseHelpers::uniformCentered<T>());
        else if (type == Normal)
            NoiseHelpers::processRandom<T> (randContext, rand, NoiseHelpers::normal<T>());
        else if (type == Pink)
            NoiseHelpers::processRandom<T> (randContext, rand, pink);

        // apply gain to random block
        applyGain (randSubBlock);

        // copy input to output if needed
        if (context.usesSeparateInputAndOutputBlocks())
            outBlock.copyFrom (inBlock);

        // add random to output
        outBlock += randBlock;
    }

    template <typename T>
    void noise<T>::processBlock (juce::dsp::AudioBlock<T>& block) noexcept
    {
        process (juce::dsp::ProcessContextReplacing<T> { block });
    }
}
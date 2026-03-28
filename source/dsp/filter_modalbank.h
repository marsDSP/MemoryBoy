#pragma once

#ifndef MEMORYBOY_FILTER_MODALBANK_H
#define MEMORYBOY_FILTER_MODALBANK_H

#include "utils/coreutil.h"
#include <complex>
#include <array>
#include <type_traits>
#include <gcem.hpp>
#include <xsimd/xsimd.hpp>
#include "filter_modal.h"
#include "math/buffer_head.h"
#include "math/buffer_view.h"
#include <JuceHeader.h>

namespace MarsDSP::DSP
{
    template<size_t maxNumModes, typename SampleType = float>
    class ModalFilterBank
    {
    public:
        static_assert(std::is_floating_point_v<SampleType>, "SampleType must be a floating point type!");

        using Vec = xsimd::batch<SampleType>;
        static constexpr auto vecSize = Vec::size;
        static constexpr auto maxNumVecModes = Buffers::buffers_detail::ceiling_divide(maxNumModes, vecSize);

        ModalFilterBank() = default;


        void setModeAmplitudes(const SampleType (&ampsReal)[maxNumModes], const SampleType (&ampsImag)[maxNumModes],
                               SampleType normalize = 1.0f);


        void setModeAmplitudes(const std::complex<SampleType> (&amps)[maxNumModes], SampleType normalize = 1.0f);


        void setModeFrequencies(const SampleType (&baseFrequencies)[maxNumModes],
                                SampleType frequencyMultiplier = static_cast<SampleType>(1));


        void setModeDecays(const SampleType (&baseTaus)[maxNumModes], SampleType originalSampleRate,
                           SampleType decayFactor = static_cast<SampleType>(1));


        void setModeDecays(const SampleType (&t60s)[maxNumModes]);


        void setNumModesToProcess(size_t numModesToProcess);


        void prepare(double sampleRate, int samplesPerBlock);


        void reset();


        auto processSample(size_t modeIndex, SampleType x) noexcept
        {
            return modes[modeIndex].processSample(x);
        }


        void process(const Buffers::BufferView<const SampleType> &buffer) noexcept;


        template<typename Modulator>
        void processWithModulation(const Buffers::BufferView<const SampleType> &block, Modulator &&modulator) noexcept;


        Buffers::BufferView<const SampleType> getRenderBuffer() const noexcept { return renderBuffer; }

    private:
        template<typename PerModeFunc, typename PerVecModeFunc>
        void doForModes(PerModeFunc &&perModeFunc, PerVecModeFunc &&perVecModeFunc);

        static Vec tau2t60(Vec tau, SampleType originalSampleRate);

        void updateAmplitudeNormalizationFactor(SampleType normalize);

        void setModeAmplitudesInternal();

        std::array<ModalFilter<Vec>, maxNumVecModes> modes;

        std::array<std::complex<SampleType>, maxNumModes> amplitudeData;
        SampleType amplitudeNormalizationFactor = static_cast<SampleType>(1);

        BufferAlloc<SampleType> renderBuffer;
        SampleType maxFreq = static_cast<SampleType>(0);
        size_t numModesToProcess = maxNumModes;
        size_t numVecModesToProcess = maxNumVecModes;

        static constexpr SampleType log1000 = gcem::log(static_cast<SampleType>(1000));

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModalFilterBank)
    };
}
#endif

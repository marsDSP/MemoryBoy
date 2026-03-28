#pragma once

#ifndef MEMORYBOY_COREUTIL_H
#define MEMORYBOY_COREUTIL_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string_view>
#include <tuple>
#include <vector>

#ifndef DOXYGEN
#if JUCE_MODULE_AVAILABLE_juce_core
#define MARSDSP_USING_JUCE 1
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#else
#define MARSDSP_USING_JUCE 0
#endif

#ifndef MARSDSP_ALLOW_TEMPLATE_INSTANTIATIONS
#define MARSDSP_ALLOW_TEMPLATE_INSTANTIATIONS 1
#endif

#ifndef MARSDSP_JASSERT_IS_CASSERT
#define MARSDSP_JASSERT_IS_CASSERT 0
#endif

#if ! MARSDSP_USING_JUCE
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 0
#include "JUCEHelpers/juce_TargetPlatform.h"
#include "JUCEHelpers/juce_CompilerWarnings.h"
#include "JUCEHelpers/juce_ExtraDefinitions.h"
#include "JUCEHelpers/juce_MathsFunctions.h"
#include "JUCEHelpers/juce_FloatVectorOperations.h"
#include "JUCEHelpers/juce_FixedSizeFunction.h"
#include "JUCEHelpers/juce_Decibels.h"
#include "JUCEHelpers/juce_SmoothedValue.h"

#if ! JUCE_MODULE_AVAILABLE_juce_dsp
#include "JUCEHelpers/dsp/juce_ProcessSpec.h"
#include "JUCEHelpers/dsp/juce_LookupTable.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wimplicit-const-int-float-conversion")
#include "JUCEHelpers/dsp/juce_FastMathApproximations.h"
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#endif
#elif JUCE_VERSION < 0x070006

namespace juce{
    template <typename Type>
    constexpr bool exactlyEqual (Type a, Type b) {
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wfloat-equal")
        return a == b;
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
    }
}
#endif
#endif


namespace MarsDSP
{
    template <typename SampleType>
    using BufferAlloc = juce::AudioBuffer<SampleType>;

    namespace CoreUtils
    {
        namespace experimental
        {
        }
    }
}
#endif
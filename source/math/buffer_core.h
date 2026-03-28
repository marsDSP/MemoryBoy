#pragma once

#ifndef MEMORYBOY_BUFFER_CORE_H
#define MEMORYBOY_BUFFER_CORE_H

#include <array>
#include <cassert>
#include <algorithm>
#include <xsimd/xsimd.hpp>

#include "utils/coreutil.h"

#ifndef jassert
#define jassert(x) assert(x)
#endif

#if MARSDSP_USING_JUCE
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#if JUCE_MODULE_AVAILABLE_juce_dsp
#include <juce_dsp/juce_dsp.h>
//#include "SIMDAudioBlock.h"
#endif
#endif

#ifndef MARSDSP_BUFFER_MAX_NUM_CHANNELS
#define MARSDSP_BUFFER_MAX_NUM_CHANNELS 32
#else

#ifndef MARSDSP_PROCESSOR_DEFAULT_CHANNEL_COUNT
#define MARSDSP_PROCESSOR_DEFAULT_CHANNEL_COUNT MARSDSP_BUFFER_MAX_NUM_CHANNELS
#endif
#endif

namespace MarsDSP::Utils::Buffers
{
    constexpr auto dynamicChannelCount = std::numeric_limits<size_t>::max();
#ifdef MARSDSP_PROCESSOR_DEFAULT_CHANNEL_COUNT
    constexpr auto defaultChannelCount = MARSDSP_PROCESSOR_DEFAULT_CHANNEL_COUNT;
#else
    constexpr auto defaultChannelCount = dynamicChannelCount;
#endif

#ifndef DOXYGEN
#endif
}

#include "buffer_detail.h"
#include "buffer_head.h"
#include "buffer_static.h"
#include "buffer_view.h"
#include "buffer_helpers.h"
#include "buffer_helpers_simd.h"
#include "buffer_iters.h"

#endif
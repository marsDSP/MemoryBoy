#pragma once

#ifndef MEMORYBOY_BUFFER_DETAIL_H
#define MEMORYBOY_BUFFER_DETAIL_H

#include <algorithm>
#include <type_traits>

namespace MarsDSP::Buffers::
inline buffers_detail {
    template<typename T>
    constexpr T ceiling_divide(T num, T den) { return (num + den - 1) / den; }

    template<typename T>
    static constexpr bool IsFloatOrDouble = std::is_same_v<T, float> || std::is_same_v<T, double>;

    template<typename SampleType>
    void clear(SampleType **channelPointers, int startChannel, int endChannel, int startSample, int endSample)
    {
        for (int channelIndex = startChannel; channelIndex < endChannel; ++channelIndex)
        {
            if (channelPointers[channelIndex] != nullptr)
            {
                std::fill(channelPointers[channelIndex] + startSample, channelPointers[channelIndex] + endSample,
                          SampleType{});
            }
        }
    }
}
#endif

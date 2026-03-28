#include "diffusor.h"

#include <algorithm>
#include <numeric>

namespace MarsDSP::DSP
{
    double DefaultDiffuserConfig::getDelayMult (int channelIndex, int nChannels, std::mt19937& mt)
{
    const auto rangeLow = static_cast<double>(channelIndex + 1) / static_cast<double>(nChannels + 1);
    const auto rangeHigh = static_cast<double>(channelIndex + 2) / static_cast<double>(nChannels + 1);

    auto&& dist = std::uniform_real_distribution<> (rangeLow, rangeHigh);
    return dist (mt);
}

double DefaultDiffuserConfig::getPolarityMultiplier (int /*channelIndex*/, int /*nChannels*/, std::mt19937& mt)
{
    auto&& dist = std::uniform_int_distribution<> (0, 1);
    return dist (mt) == 0 ? 1.0 : -1.0;
}

void DefaultDiffuserConfig::fillChannelSwapIndexes (size_t* indexes, int numChannels, std::mt19937& mt)
{
    std::iota (indexes, indexes + numChannels, 0);
    std::shuffle (indexes, indexes + numChannels, mt);
}

}
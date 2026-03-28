#include "coreutil.h"

#if ! MARSDSP_USING_JUCE
#include "JUCEHelpers/juce_FloatVectorOperations.cpp"

#if ! JUCE_MODULE_AVAILABLE_juce_dsp
#include "JUCEHelpers/dsp/juce_LookupTable.cpp"
#endif
#endif
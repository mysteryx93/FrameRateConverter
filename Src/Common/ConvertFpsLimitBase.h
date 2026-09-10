#pragma once
#include "../Environments/Common.h"
#include "merge.h"
#include <stdint.h>
#include <cmath>

// Class to change the framerate, attempting to smooth the transitions
class ConvertFPSLimitBase
{
public:
	static const char* PluginName;
	ConvertFPSLimitBase(ICommonVideo* _child, const ICommonEnvironment& env, int new_numerator, int new_denominator, int _ratio);
	const ICommonFrame& ProcessFrame(int n, const ICommonFrame& src, const ICommonFrame& srcNext, const ICommonEnvironment& env);

protected:
	ICommonVideo* source;
	int64_t fa, fb;
	int ratio;

private:
	static bool float_to_frac(float input, unsigned& num, unsigned& den);
	static bool reduce_float(float value, unsigned& num, unsigned& den);
	static void reduce_frac(uint32_t& num, uint32_t& den, uint32_t limit);
public:
	static void FloatToFPS(float n, uint32_t& num, uint32_t& den, const ICommonEnvironment& env);
	static void PresetToFPS(const char* p, uint32_t& num, uint32_t& den, const ICommonEnvironment& env);
};

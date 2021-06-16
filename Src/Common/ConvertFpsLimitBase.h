#pragma once
#include "../Environments/Common.h"
#include "ConvertFpsFunc.h"
#include "merge.h"
#include <stdint.h>
#include <cmath>
#include <Windows.h>

// Class to change the framerate, attempting to smooth the transitions
class ConvertFPSLimitBase
{
public:
	ConvertFPSLimitBase(ICommonVideo* _child, unsigned new_numerator, unsigned new_denominator, int _ratio, ICommonEnvironment& env);
	ICommonFrame& ProcessFrame(int n, ICommonFrame& src, ICommonFrame& srcNext, ICommonEnvironment& env);

protected:
	ICommonVideo* source;
	int64_t fa, fb;
	int ratio;
};

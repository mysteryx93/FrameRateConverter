#pragma once
#include "../Environments/Common.h"
#include <cstring>
#include <cstdint>
#include <Windows.h>

class ContinuousMaskBase
{
private:
	ICommonEnvironment& env;
	const int width;
	const int height;
	const int radius;
	const int bitsPerSample;

protected:
	ContinuousMaskBase(ICommonVideo* _clip, ICommonEnvironment& _env, int _radius);
	~ContinuousMaskBase() {}
	void ProcessFrame(ICommonFrame& src, ICommonFrame& dst);

	// T: data type to calculate total (must hold P.MaxValue * radius * 4)
	// P: data type of each pixel
	template<typename T, typename P> void Calculate(const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch);
};
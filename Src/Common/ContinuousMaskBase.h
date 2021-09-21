#pragma once
#include "../Environments/Common.h"
#include <cstring>
#include <cstdint>
#include <Windows.h>

class ContinuousMaskBase
{
public:
	static const char* PluginName;

private:
	// ICommonEnvironment& env;
	const int radius;
	const int thr;
	const int bitsPerSample;

protected:
	ContinuousMaskBase(ICommonVideo* _child, ICommonEnvironment& _env, int _radius, int _thr);
	~ContinuousMaskBase() {}
	void ProcessFrame(ICommonFrame& src, ICommonFrame& dst);
	ICommonVideo* source;

	// T: data type to calculate total (must hold P.MaxValue * radius * 4)
	// P: data type of each pixel
	template<typename T, typename P> void Calculate(int width, int height, const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch);
};

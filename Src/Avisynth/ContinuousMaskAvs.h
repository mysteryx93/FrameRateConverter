#pragma once
#include "../Environments/Avisynth.hpp"
#include "../Common/ContinuousMaskBase.h"

class ContinuousMask : public GenericVideoFilter, public ContinuousMaskBase
{
public:
	ContinuousMask(PClip _child, int _radius, IScriptEnvironment* env);
	~ContinuousMask() {}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	int __stdcall SetCacheHints(int cachehints, int frame_range);
};

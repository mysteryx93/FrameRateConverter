#pragma once
#include "../Environments/Avisynth.hpp"
#include "../Common/ContinuousMaskBase.h"

class ContinuousMaskAvs : public GenericVideoFilter, public ContinuousMaskBase
{
public:
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	ContinuousMaskAvs(PClip _child, int _radius, int _thr, IScriptEnvironment* env);
	~ContinuousMaskAvs() {}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	int __stdcall SetCacheHints(int cachehints, int frame_range);
};

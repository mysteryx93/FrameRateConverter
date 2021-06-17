#pragma once
#include "../Common/ConvertFpsLimitBase.h"
#include "../Environments/Avisynth.hpp"

class ConvertFpsLimitAvs: public GenericVideoFilter, public ConvertFPSLimitBase
	/**
	  * Class to change the framerate, attempting to smooth the transitions
	 **/
{
public:
	ConvertFpsLimitAvs(PClip _child, unsigned new_numerator, unsigned new_denominator, int _ratio, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

	int __stdcall SetCacheHints(int cachehints, int frame_range) override {
		return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
	}

	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreateFloat(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreatePreset(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreateFromClip(AVSValue args, void*, IScriptEnvironment* env);
};

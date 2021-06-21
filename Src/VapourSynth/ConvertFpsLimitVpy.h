#pragma once
#include "../Common/ConvertFpsLimitBase.h"
#include "../Environments/VpyFilter.hpp"
#include "../Environments/VpyPropReader.hpp"

class ConvertFpsLimitVpy: public VpyFilter, ConvertFPSLimitBase
	/**
	  * Class to change the framerate, attempting to smooth the transitions
	 **/
{
public:
	static void VS_CC Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	static void VS_CC CreateFloat(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	static void VS_CC CreatePreset(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	static void VS_CC CreateFromClip(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);

	ConvertFpsLimitVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* vsapi, int new_numerator, int new_denominator, int _ratio);
	~ConvertFpsLimitVpy() {}
	void VpyFilter::Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env);
	VSFrameRef* VpyFilter::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env);
	void VpyFilter::Free();
};

unsigned int gcd(unsigned int u, unsigned int v);
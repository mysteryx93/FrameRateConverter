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

	ConvertFpsLimitVpy(const VSMap* in, VSMap* out, VSNode* node, VSCore* core, const VSAPI* vsapi, uint32_t new_numerator, uint32_t new_denominator, int _ratio);
	~ConvertFpsLimitVpy() {}
	const VSFrame* GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env) override;
	void Free() override;
};

uint32_t gcd(uint32_t u, uint32_t v);

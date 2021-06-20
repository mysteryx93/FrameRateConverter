#pragma once
#include "../Environments/VpyFilter.hpp"
#include "../Common/ContinuousMaskBase.h"

class ContinuousMaskVpy : public VpyFilter, public ContinuousMaskBase
{
public:
	static void VS_CC Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* vsapi, int _radius);
	~ContinuousMaskVpy() {}
	void VpyFilter::Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env);
	VSFrameRef* VpyFilter::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env);
	void VpyFilter::Free();
};

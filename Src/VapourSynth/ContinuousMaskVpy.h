#pragma once
#include "../Environments/VpyFilter.hpp"
#include "../Common/ContinuousMaskBase.h"

class ContinuousMaskVpy : public VpyFilter, public ContinuousMaskBase
{
public:
	static void VS_CC Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* vsapi, int _radius, int _thr);
	~ContinuousMaskVpy() {}
	void Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env) override;
	VSFrameRef* GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env) override;
	void Free() override;
};

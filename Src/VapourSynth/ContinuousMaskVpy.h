#pragma once
#include "../Environments/VapourSynthFilter.hpp"
#include "../Common/ContinuousMaskBase.h"

class ContinuousMaskVpy : public VapourSynthFilter, public ContinuousMaskBase
{
public:
	static void VS_CC ContinuousMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);

	ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* vsapi, int _radius);
	~ContinuousMaskVpy() {}

	void VapourSynthFilter::Init(VSMap* in, VSMap* out, VSNode* node, VSCore* core, const VSAPI* api);
	VSFrameRef* VapourSynthFilter::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api);
	void VapourSynthFilter::Free(VSCore* core, const VSAPI* api);

	//static void VS_CC Init(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* vsapi);
	//static const VSFrameRef* VS_CC GetFrame(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi);
	//static void VS_CC Free(void* instanceData, VSCore* core, const VSAPI* vsapi);
};

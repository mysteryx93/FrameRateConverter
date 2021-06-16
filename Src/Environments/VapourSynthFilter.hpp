#pragma once
#include "VapourSynth.hpp"

// Base class for VapourSynth filter where it initializes and calls a class. The class pointer is stored in instanceData, and the class itself holds the data.
class VapourSynthFilter
{
public:
	VSNodeRef* Node;
	const VSVideoInfo* VInfo;

	VapourSynthFilter::VapourSynthFilter(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* api) :
		Node(node), VInfo(api->getVideoInfo(node))
	{
		api->createFilter(in, out, "ContinuousMask", VapourSynthFilter::Init, VapourSynthFilter::GetFrame, VapourSynthFilter::Free, fmParallel, 0, this, core);
	}

	~VapourSynthFilter() {}

	virtual void VapourSynthFilter::Init(VSMap* in, VSMap* out, VSNode* node, VSCore* core, const VSAPI* api) = 0;
	virtual VSFrameRef* VapourSynthFilter::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api) = 0;
	virtual void VapourSynthFilter::Free(VSCore* core, const VSAPI* api) = 0;

	static void VS_CC VapourSynthFilter::Init(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* api)
	{
		VapourSynthFilter* filter = (VapourSynthFilter*)*instanceData;
		filter->Init(in, out, node, core, api);
	}

	static const VSFrameRef* VS_CC VapourSynthFilter::GetFrame(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api)
	{
		VapourSynthFilter* filter = (VapourSynthFilter*)*instanceData;
		return filter->GetFrame(n, activationReason, frameData, frameCtx, core, api);
	}

	static void VS_CC VapourSynthFilter::Free(void* instanceData, VSCore* core, const VSAPI* api)
	{
		VapourSynthFilter* filter = (VapourSynthFilter*)instanceData;
		filter->Free(core, api);
		api->freeNode(filter->Node);
		delete filter;
	}
};

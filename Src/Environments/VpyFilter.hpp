#pragma once
#include "VapourSynth.hpp"
#include "VpyPropReader.hpp"

// Base class for VapourSynth filter where it initializes and calls a class. The class pointer is stored in instanceData, and the class itself holds the data.
class VpyFilter
{
public:
	const char* PluginName2;
	VSNodeRef* Node;
	const VSVideoInfo* viSrc;
	VSVideoInfo viDst;
	// VpyEnvironment env;
	VSCore* core;
	const VSAPI* api;
	VSMap* Out;

	VpyFilter::VpyFilter(const char* pluginName, const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* _core, const VSAPI* _api) :
		Node(node), viSrc(_api->getVideoInfo(node)), viDst(*viSrc), api(_api), core(_core), Out(out)
	{
		PluginName2 = pluginName;
	}

	~VpyFilter()
	{
	}

	void CreateFilter(const VSMap* in, VSMap* out)
	{
		api->createFilter(in, out, "ContinuousMask", VpyFilter::Init, VpyFilter::GetFrame, VpyFilter::Free, fmParallel, 0, this, core);
	}

	bool HasError()
	{
		return api->getError(Out);
	}

	virtual void VpyFilter::Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env) = 0;
	virtual VSFrameRef* VpyFilter::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env) = 0;
	virtual void VpyFilter::Free() = 0;



	static void VS_CC VpyFilter::Init(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)*instanceData;
		filter->Init(in, out, node, VpyEnvironment(filter->PluginName2, api, core, out));
		api->setVideoInfo(&filter->viDst, 1, node);
	}

	static const VSFrameRef* VS_CC VpyFilter::GetFrame(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)*instanceData;
		return filter->GetFrame(n, activationReason, frameData, frameCtx, VpyEnvironment(filter->PluginName2, api, core, frameCtx));
	}

	static void VS_CC VpyFilter::Free(void* instanceData, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)instanceData;
		filter->Free();
		api->freeNode(filter->Node);
		delete filter;
	}
};

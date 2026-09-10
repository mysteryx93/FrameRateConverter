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

	VpyFilter(const char* pluginName, const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* _core, const VSAPI* _api) :
		PluginName2(pluginName), Node(node), viSrc(_api->getVideoInfo(node)), viDst(*viSrc), core(_core), api(_api), Out(out)
	{
	}

	virtual ~VpyFilter()
	{
	}

	void CreateFilter(const VSMap* in, VSMap* out)
	{
		api->createFilter(in, out, PluginName2, FilterInit, FilterGetFrame, FilterFree, fmParallel, 0, this, core);
	}

	bool HasError()
	{
		return api->getError(Out);
	}

	virtual void Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env) = 0;
	virtual VSFrameRef* GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env) = 0;
	virtual void Free() = 0;

	static void VS_CC FilterInit(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)*instanceData;
		VpyEnvironment env(filter->PluginName2, api, core, out);
		filter->Init(in, out, node, env);
		api->setVideoInfo(&filter->viDst, 1, node);
	}

	static const VSFrameRef* VS_CC FilterGetFrame(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)*instanceData;
		filter->core = core;
		VpyEnvironment env(filter->PluginName2, api, core, frameCtx);
		return filter->GetFrame(n, activationReason, frameData, frameCtx, env);
	}

	static void VS_CC FilterFree(void* instanceData, VSCore* core, const VSAPI* api)
	{
		VpyFilter* filter = (VpyFilter*)instanceData;
		filter->core = core;
		filter->Free();
		api->freeNode(filter->Node);
		delete filter;
	}
};

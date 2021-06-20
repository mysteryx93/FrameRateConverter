#include "ConvertFpsLimitVpy.h"

ConvertFpsLimitVpy::ConvertFpsLimitVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* api, 
	int new_numerator, int new_denominator, int _ratio) :
	VpyFilter(PluginName, in, out, node, core, api),
	ConvertFPSLimitBase(new VpyVideo(node, api), VpyEnvironment(PluginName, api, core, out), new_numerator, new_denominator, _ratio)
{
	viDst.fpsNum = new_numerator;
	viDst.fpsDen = new_denominator;
	const int64_t num_frames = (viSrc->numFrames * fb + (fa >> 1)) / fa;
	if (num_frames > 0x7FFFFFFF)  // MAXINT
		api->setError(out, "ConvertFpsLimit: Maximum number of frames exceeded.");
	viDst.numFrames = int(num_frames);
}

void ConvertFpsLimitVpy::Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env)
{
}

VSFrameRef* ConvertFpsLimitVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env)
{
	if (activationReason == arInitial)
	{
		int nsrc = int(n * fa / fb);
		api->requestFrameFilter(nsrc, Node, frameCtx);
		api->requestFrameFilter(nsrc + 1, Node, frameCtx);
	}
	else if (activationReason == arAllFramesReady)
	{
		int nsrc = int(n * fa / fb);
		auto src = api->getFrameFilter(nsrc, Node, frameCtx);
		auto src2 = api->getFrameFilter(nsrc + 1, Node, frameCtx);
		auto srcCopy = api->copyFrame(src, core);
		api->freeFrame(src);

		auto dst = (VSFrameRef*)ProcessFrame(n, VpyFrame(srcCopy, api), VpyFrame(src2, api), env).Ref;

		api->freeFrame(dst == srcCopy ? src2 : srcCopy);
		return dst;
	}

	return nullptr;
}

void ConvertFpsLimitVpy::Free()
{
}


void VS_CC ConvertFpsLimitVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* src = prop.GetNode("clip");
	int num = prop.GetInt("num");
	int den = prop.GetInt("den");
	int ratio = prop.GetInt("ratio", 100);
	auto f = new ConvertFpsLimitVpy(in, out, src, core, api, num, den, ratio);
	f->CreateFilter(in, out);
}

void VS_CC ConvertFpsLimitVpy::CreateFloat(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* src = prop.GetNode("clip");
	float fps = prop.GetInt("fps");
	int ratio = prop.GetInt("ratio", 100);

	uint32_t num, den;
	FloatToFPS(fps, num, den, VpyEnvironment("ConvertFpsLimit", api, core, out));
	auto f = new ConvertFpsLimitVpy(in, out, src, core, api, num, den, ratio);
	f->CreateFilter(in, out);
}

// Tritical Jan 2006
void VS_CC ConvertFpsLimitVpy::CreatePreset(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* src = prop.GetNode("clip");
	const char* fps = prop.GetData("fps");
	int ratio = prop.GetInt("ratio", 100);

	uint32_t num, den;
	PresetToFPS(fps, num, den, VpyEnvironment("ConvertFpsLimit", api, core, out));
	auto f = new ConvertFpsLimitVpy(in, out, src, core, api, num, den, ratio);
	f->CreateFilter(in, out);
}

void VS_CC ConvertFpsLimitVpy::CreateFromClip(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* src = prop.GetNode("clip");
	auto fps = prop.GetNode("fps");
	int ratio = prop.GetInt("ratio", 100);

	auto vi = api->getVideoInfo(fps);
	auto f = new ConvertFpsLimitVpy(in, out, src, core, api, vi->fpsNum, vi->fpsDen, ratio);
	f->CreateFilter(in, out);
}

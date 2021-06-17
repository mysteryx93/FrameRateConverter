#include "StripeMaskVpy.h"

void VS_CC StripeMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	//new StripeMaskVpy(in, out, src, core, api, radius);

	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* Input = prop.GetNode("clip");
	int BlkSize = prop.GetInt("blkSize", 16);
	int BlkSizeV = prop.GetInt("blkSizeV", BlkSize > 0 ? BlkSize : 16);
	int Overlap = prop.GetInt("overlap", BlkSize / 4);
	int OverlapV = prop.GetInt("overlapV", BlkSizeV / 4);
	int Thr = prop.GetInt("thr", 26);
	int Comp = prop.GetInt("comp", BlkSize <= 16 ? 2 : 3);
	int CompV = prop.GetInt("compV", Comp);
	int Str = prop.GetInt("str", 255);
	int Strf = prop.GetInt("strf", 0);
	bool Lines = prop.GetInt("lines", false);

	//// Convert input to linear (gamma 2.2)
	//AVSValue sargs[2] = { Input, ((1.0 / 2.2) - 1.0) * 256.0 };
	//const char* nargs[2] = { 0, "gamma_y" };
	//Input = env->Invoke("ColorYUV", AVSValue(sargs, 2), nargs).AsClip();

	auto Vi = api->getVideoInfo(Input);
	bool ReduceBits = Vi->format->bitsPerSample > 8;

	//// Convert input to 8-bit; nothing to gain in processing at higher bit-depth.
	VSPlugin* Plugin = api->getPluginByNs("resize", core);
	if (ReduceBits)
	{
		VSMap* Args = api->createMap();
		api->propSetNode(Args, "clip", Input, paReplace);
		api->propSetInt(Args, "format", pfGray8, paReplace);
		out = api->invoke(Plugin, "Bicubic", Args);
		api->freeMap(Args);
	}

	auto f = new StripeMaskVpy(in, out, core, api, Input, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, Strf, Lines);
	f->CreateFilter(in, out);

	//// Convert back to original bit depth.
	if (ReduceBits)
	{
		VSMap* Args = api->createMap();
		api->propSetNode(Args, "clip", Input, paReplace);
		api->propSetInt(Args, "format", Vi->format->id, paReplace);
		out = api->invoke(Plugin, "Bicubic", Args);
		api->freeMap(Args);
	}
}

StripeMaskVpy::StripeMaskVpy(const VSMap* in, VSMap* out, VSCore* core, const VSAPI* api, VSNodeRef* node,
	int blkSize, int blkSizeV, int overlap, int overlapV, int thr, int comp, int compV, int str, int strf, bool lines) :
	VpyFilter(in, out, node, core, api),
	StripeMaskBase(new VpyVideo(node, api), VpyEnvironment(api, core), blkSize, blkSizeV, overlap, overlapV, thr, comp, compV, str, strf, lines)
{
	int b = viSrc->format->bitsPerSample;
	viDst.format = api->getFormatPreset(b <= 8 ? pfGray8 : b <= 16 ? pfGray16 : b == 32 ? pfGrayS : pfGray8, core);
}

void StripeMaskVpy::Init(VSMap* in, VSMap* out, VSNode* node)
{
}

VSFrameRef* StripeMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx)
{
	if (activationReason == arInitial)
	{
		api->requestFrameFilter(n, Node, frameCtx);
	}
	else if (activationReason == arAllFramesReady)
	{
		const VSFrameRef* src = api->getFrameFilter(n, Node, frameCtx);
		const VSFrameRef* src2 = nullptr;
		if (strf > 0 && n < viSrc->numFrames - 1)
		{
			src2 = api->getFrameFilter(n + 1, Node, frameCtx);
		}
		VSFrameRef* dst = api->newVideoFrame(viDst.format, viDst.width, viDst.height, src, core);

		ProcessFrame(VpyFrame(src, api), VpyFrame(src2, api), VpyFrame(dst, api), VpyEnvironment(api, core));

		api->freeFrame(src);
		return dst;
	}

	return nullptr;
}

void StripeMaskVpy::Free()
{
}

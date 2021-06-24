#include "StripeMaskVpy.h"

void VS_CC StripeMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	auto Input = prop.GetNode("clip");
	int BlkSize = prop.GetInt("blksize", 16);
	int BlkSizeV = prop.GetInt("blksizev", BlkSize > 0 ? BlkSize : 16);
	int Overlap = prop.GetInt("overlap", BlkSize / 4);
	int OverlapV = prop.GetInt("overlapv", BlkSizeV / 4);
	int Thr = prop.GetInt("thr", 28);
	int Comp = prop.GetInt("comp", BlkSize <= 16 ? 2 : 3);
	int CompV = prop.GetInt("compv", Comp);
	int Str = prop.GetInt("str", 255);
	int Strf = prop.GetInt("strf", 0);
	bool FullRangeDef = prop.GetInt("_ColorRange", 0) == 1;
	bool FullRange = prop.GetInt("fullrange", FullRangeDef);
	bool Lines = prop.GetInt("lines", false);

	auto Vi = api->getVideoInfo(Input);
	int BitDepth = Vi->format->bitsPerSample;

	// FMTC transfer only supports Gray16 or RGBS format.
	VpyEnvironment Env = VpyEnvironment(StripeMaskBase::PluginName, api, core, out);
	VSMap* Args = api->createMap();
	api->propSetNode(Args, "clip", Input, paReplace);
	api->propSetInt(Args, "format", pfGray16, paReplace);
	Input = Env.InvokeClip("resize", "Point", Args, Input);
	if (Input)
	{
		// Convert to linear light.
		api->clearMap(Args);
		api->propSetNode(Args, "clip", Input, paReplace);
		api->propSetData(Args, "transs", "709", -1, paReplace);
		api->propSetData(Args, "transd", "linear", -1, paReplace);
		api->propSetInt(Args, "fulls", FullRange ? 1 : 0, paReplace);
		Input = Env.InvokeClip("fmtc", "transfer", Args, Input);
		if (Input)
		{
			// Convert to GRAY8 for processing.
			api->clearMap(Args);
			api->propSetNode(Args, "clip", Input, paReplace);
			api->propSetInt(Args, "format", pfGray8, paReplace);
			Input = Env.InvokeClip("resize", "Point", Args, Input);
			if (Input)
			{
				auto f = new StripeMaskVpy(in, out, core, api, Input, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, Strf, Lines);
				f->CreateFilter(in, out);

				if (!f->HasError())
				{
					VpyPropReader FilterProp = VpyPropReader(api, out);
					if (BitDepth > 8)
					{
						Input = FilterProp.GetNode("clip");
						// Convert back to original bit depth.
						api->clearMap(Args);
						api->propSetNode(Args, "clip", Input, paReplace);
						api->propSetInt(Args, "format", BitDepth <= 16 ? pfGray16 : pfGrayS, paReplace);
						Input = Env.InvokeClip("resize", "Point", Args, Input);

						api->propSetNode(out, "clip", Input, paReplace);
						api->freeNode(Input);
					}
				}
			}
		}
	}

	api->freeMap(Args);
}

StripeMaskVpy::StripeMaskVpy(const VSMap* in, VSMap* out, VSCore* core, const VSAPI* api, VSNodeRef* node,
	int blkSize, int blkSizeV, int overlap, int overlapV, int thr, int comp, int compV, int str, int strf, bool lines) :
	VpyFilter(PluginName, in, out, node, core, api),
	StripeMaskBase(new VpyVideo(node, api), VpyEnvironment(PluginName, api, core, out), blkSize, blkSizeV, overlap, overlapV, thr, comp, compV, str, strf, lines)
{
	int b = viSrc->format->bitsPerSample;
	viDst.format = api->getFormatPreset(b <= 8 ? pfGray8 : b <= 16 ? pfGray16 : b == 32 ? pfGrayS : pfGray8, core);
}

void StripeMaskVpy::Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env)
{
}

VSFrameRef* StripeMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env)
{
	if (activationReason == arInitial)
	{
		api->requestFrameFilter(n, Node, frameCtx);
		if (strf > 0 && n < viSrc->numFrames - 1)
		{
			api->requestFrameFilter(n + 1, Node, frameCtx);
		}
	}
	else if (activationReason == arAllFramesReady)
	{
		const VSFrameRef* src = api->getFrameFilter(n, Node, frameCtx);
		const VSFrameRef* src2 = nullptr;
		if (strf > 0 && n < viSrc->numFrames - 1)
		{
			src2 = api->getFrameFilter(n + 1, Node, frameCtx);
		}
		//auto dst = api->copyFrame(src, core);

		VSFrameRef* dst = api->newVideoFrame(viDst.format, viDst.width, viDst.height, src, core);
		ProcessFrame(VpyFrame(src, api), VpyFrame(src2, api), VpyFrame(dst, api), env);

		api->freeFrame(src);
		if (src2)
		{
			api->freeFrame(src2);
		}
		return dst;
	}

	return nullptr;
}

void StripeMaskVpy::Free()
{
}

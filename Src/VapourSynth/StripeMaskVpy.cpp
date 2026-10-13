#include "StripeMaskVpy.h"

void VS_CC StripeMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	auto Input = prop.GetNode("clip");
	int BlkSize = prop.GetInt("blksize", 16);
	int BlkSizeV = prop.GetInt("blksizev", BlkSize > 0 ? BlkSize : 16);
	int Overlap = prop.GetInt("overlap", BlkSize / 4);
	int OverlapV = prop.GetInt("overlapv", BlkSizeV / 4);
	int Thr = prop.GetInt("thr", 15);
	int Range = prop.GetInt("range", 125);
	double Gamma = prop.GetFloat("gamma", 2.2);
	int Comp = prop.GetInt("comp", BlkSize <= 16 ? 2 : 3);
	int CompV = prop.GetInt("compv", Comp);
	int Str = prop.GetInt("str", 255);
	//int Strf = prop.GetInt("strf", 0);
	bool SrcFullRange = prop.GetInt("fullrange", 0) != 0;
	bool Lines = prop.GetInt("lines", false);

	auto Vi = api->getVideoInfo(Input);
	int BitDepth = Vi->format.bitsPerSample;
	int SampleType = Vi->format.sampleType;

	VpyEnvironment Env = VpyEnvironment(PluginName, api, core, out);
	if (Vi->format.colorFamily != cfYUV && Vi->format.colorFamily != cfGray)
	{
		Env.ThrowError("clip must be Y or YUV format");
	}
	else if (Vi->width <= 0 || Vi->height <= 0)
	{
		Env.ThrowError("clip must have constant dimensions.");
	}
	else if (Range < 1 || Range > 255)
	{
		Env.ThrowError("range must be between 1 and 255.");
	}
	else if (Gamma <= 0 || Gamma > 100)
	{
		Env.ThrowError("gamma must be greater than 0 and less than 100.");
	}
	else if (BlkSize < 0 || BlkSizeV < 0)
	{
		Env.ThrowError("blksize and blksizev must be 0 or above.");
	}
	else if ((BlkSize != 0 && (Overlap < 0 || Overlap >= BlkSize)) ||
		(BlkSizeV != 0 && (OverlapV < 0 || OverlapV >= BlkSizeV)))
	{
		Env.ThrowError("overlap must be non-negative and smaller than its block size.");
	}
	else if (Thr < 0 || Thr > 255)
	{
		Env.ThrowError("thr must be between 0 and 255.");
	}
	else if (Comp < 2 || Comp > 5 || CompV < 2 || CompV > 5)
	{
		Env.ThrowError("comp and compv must be between 2 and 5.");
	}
	else if (Str < 0 || Str > 255)
	{
		Env.ThrowError("str must be between 0 and 255.");
	}
	if (api->mapGetError(out))
	{
		api->freeNode(Input);
		return;
	}

	// Keep only Luma plane.
	VSMap* Args = api->createMap();
	{
		api->mapSetNode(Args, "clips", Input, maReplace);
		api->mapSetInt(Args, "planes", 0, maReplace);
		api->mapSetInt(Args, "colorfamily", cfGray, maReplace);
		Input = Env.InvokeClip("std", "ShufflePlanes", Args, Input);
	}

	// Convert to 8-bit, preserving TV vs PC range (zimg: 0=full, 1=limited).
	if (Input && BitDepth > 8)
	{
		int Rng = SrcFullRange ? 0 : 1;
		api->clearMap(Args);
		api->mapSetNode(Args, "clip", Input, maReplace);
		api->mapSetInt(Args, "format", pfGray8, maReplace);
		api->mapSetInt(Args, "range_in", Rng, maReplace);
		api->mapSetInt(Args, "range", Rng, maReplace);
		Input = Env.InvokeClip("resize", "Point", Args, Input);
	}

	// Reduce range and apply gamma.
	if (Input)
	{
		api->clearMap(Args);
		api->mapSetNode(Args, "clip", Input, maReplace);
		api->mapSetFloat(Args, "gamma", 1.0 / Gamma, maReplace);
		api->mapSetFloat(Args, "min_in", SrcFullRange ? 0 : 16, maReplace);
		api->mapSetFloat(Args, "max_in", SrcFullRange ? 255 : 235, maReplace);
		api->mapSetFloat(Args, "min_out", 0, maReplace);
		api->mapSetFloat(Args, "max_out", Range, maReplace);
		Input = Env.InvokeClip("std", "Levels", Args, Input);
	}

	// Create StripeMask.
	if (Input)
	{
		auto f = new StripeMaskVpy(in, out, core, api, Input, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, Lines);
		f->CreateFilter(in, out);

		if (!f->HasError() && BitDepth > 8)
		{
			uint32_t Format = api->queryVideoFormatID(cfGray, SampleType, BitDepth, 0, 0, core);
			if (!Format)
			{
				Env.ThrowError("cannot restore the source bit depth.");
			}
			else
			{
				// Convert back to original bit depth.
				VSNode* Input2 = VpyPropReader(api, out).GetNode("clip");
				api->clearMap(Args);
				api->mapSetNode(Args, "clip", Input2, maReplace);
				api->mapSetInt(Args, "format", Format, maReplace);
				api->mapSetInt(Args, "range_in", 1, maReplace);
				api->mapSetInt(Args, "range", 1, maReplace);
				Input2 = Env.InvokeClip("resize", "Point", Args, Input2);
				if (Input2)
				{
					api->mapSetNode(out, "clip", Input2, maReplace);
					api->freeNode(Input2);
				}
			}
		}
	}

	api->freeMap(Args);
}

StripeMaskVpy::StripeMaskVpy(const VSMap* in, VSMap* out, VSCore* core, const VSAPI* api, VSNode* node,
	int blkSize, int blkSizeV, int overlap, int overlapV, int thr, int comp, int compV, int str, bool lines) :
	VpyFilter(PluginName, in, out, node, core, api),
	StripeMaskBase(new VpyVideo(node, api), VpyEnvironment(PluginName, api, core, out), blkSize, blkSizeV, overlap, overlapV, thr, comp, compV, str, lines)
{
	api->getVideoFormatByID(&viDst.format, pfGray8, core);
}

const VSFrame* StripeMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env)
{
	if (activationReason == arInitial)
	{
		api->requestFrameFilter(n, Node, frameCtx);
	}
	else if (activationReason == arAllFramesReady)
	{
		const VSFrame* src = api->getFrameFilter(n, Node, frameCtx);
		VSFrame* dst = api->newVideoFrame(&viDst.format, viDst.width, viDst.height, src, core);
		ProcessFrame(VpyFrame(src, api), VpyFrame(dst, api), env);

		api->freeFrame(src);
		return dst;
	}

	return nullptr;
}

void StripeMaskVpy::Free()
{
}

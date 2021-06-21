#include "ConvertFpsLimitVpy.h"

void VS_CC ConvertFpsLimitVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	VpyPropReader prop = VpyPropReader(api, in);
	VSNodeRef* src = prop.GetNode("clip");
	uint32_t num = prop.GetInt("num");
	uint32_t den = prop.GetInt("den");
	float fps = prop.GetFloat("fps");
	const char* preset = prop.GetData("preset");
	VSNodeRef* match = prop.GetNode("match");
	int ratio = prop.GetInt("ratio", 100);

	int ParamCount = 0;
	if (num)
		ParamCount++;
	if (fps)
		ParamCount++;
	if (preset)
		ParamCount++;
	if (match)
		ParamCount++;

	VpyEnvironment Env = VpyEnvironment(PluginName, api, core, out);
	if (ParamCount == 0)
	{
		Env.ThrowError("Must set one of the following parameters: num/den (fraction), fps (float), preset (string) or match (clip).");
	}
	else if ((num > 0) != (den > 0))
	{
		Env.ThrowError("Both num and den must be specified.");
	}
	else if (ParamCount > 1)
	{
		Env.ThrowError("Can only set one of the following parameters: num/den (fraction), fps (float), preset (string) or match (clip).");
	}
	else
	{
		if (fps)
		{
			FloatToFPS(fps, num, den, Env);
		}
		else if (preset)
		{
			PresetToFPS(preset, num, den, Env);
		}
		else if (match)
		{
			auto vi = api->getVideoInfo(match);
			num = vi->fpsNum;
			den = vi->fpsDen;
			api->freeNode(match);
		}

		// VapourSynth only accepts reduced fractions.
		unsigned int div = gcd(num, den);
		num /= div;
		den /= div;

		auto f = new ConvertFpsLimitVpy(in, out, src, core, api, num, den, ratio);
		f->CreateFilter(in, out);
	}
}

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

unsigned int gcd(unsigned int u, unsigned int v)
{
	// Base cases
	// gcd(n, n) = n
	if (u == v)
		return u;

	//  Identity 1: gcd(0, n) = gcd(n, 0) = n
	if (u == 0)
		return v;
	if (v == 0)
		return u;

	if (u & 1) { // u is odd
		if (v % 2 == 0) // v is even
			return gcd(u, v / 2); // Identity 3
		// Identities 4 and 3 (u and v are odd, so u-v and v-u are known to be even)
		if (u > v)
			return gcd((u - v) / 2, v);
		else
			return gcd((v - u) / 2, u);
	}
	else { // u is even
		if (v & 1) // v is odd
			return gcd(u / 2, v); // Identity 3
		else // both u and v are even
			return 2 * gcd(u / 2, v / 2); // Identity 2
	}
}

#include "ContinuousMaskVpy.h"

void VS_CC ContinuousMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
    VpyPropReader prop = VpyPropReader(api, in);
    VSNode* src = prop.GetNode("clip");
    int radius = prop.GetInt("radius", 16);
    int thr = prop.GetInt("thr", 0);
	VpyEnvironment env(PluginName, api, core, out);
	const auto vi = api->getVideoInfo(src);
	if (vi->format.colorFamily == cfUndefined || vi->width <= 0 || vi->height <= 0)
	{
		env.ThrowError("clip must have a constant video format and dimensions.");
		api->freeNode(src);
		return;
	}
	if (radius <= 1)
	{
		env.ThrowError("radius must be above 1");
		api->freeNode(src);
		return;
	}
	if (thr < 0 || thr > 255)
	{
		env.ThrowError("thr must be between 0 and 255.");
		api->freeNode(src);
		return;
	}

	auto f = new ContinuousMaskVpy(in, out, src, core, api, radius, thr);
    f->CreateFilter(in, out);
}

ContinuousMaskVpy::ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNode* node, VSCore* core, const VSAPI* api, int _radius, int _thr) :
    VpyFilter(PluginName, in, out, node, core, api),
	ContinuousMaskBase(new VpyVideo(node, api), VpyEnvironment(PluginName, api, core, out), _radius, _thr)
{
}

const VSFrame* ContinuousMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env)
{
    if (activationReason == arInitial)
    {
        api->requestFrameFilter(n, Node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrame* src = api->getFrameFilter(n, Node, frameCtx);
        VSFrame* dst = api->newVideoFrame(&viSrc->format, viSrc->width, viSrc->height, src, core);

        ProcessFrame(VpyFrame(src, api), VpyFrame(dst, api));

        api->freeFrame(src);
        return dst;
    }

    return nullptr;
}

void ContinuousMaskVpy::Free()
{
}

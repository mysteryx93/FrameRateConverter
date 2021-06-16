#include "ContinuousMaskVpy.h"

void VS_CC ContinuousMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
	int err;
	VSNodeRef* src = api->propGetNode(in, "clip", 0, &err);
	int radius = api->propGetInt(in, "radius", 1, &err);
	if (err)
		radius = 16;

	new ContinuousMaskVpy(in, out, src, core, api, radius);
}

ContinuousMaskVpy::ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* api, int _radius) :
	VapourSynthFilter(in, out, node, core, api),
	ContinuousMaskBase(new VpyVideo(node, api), VpyEnvironment(api, core, out), _radius)
{
}

void ContinuousMaskVpy::Init(VSMap* in, VSMap* out, VSNode* node, VSCore* core, const VSAPI* api)
{
}

VSFrameRef* ContinuousMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* api)
{
    if (activationReason == arInitial)
    {
        api->requestFrameFilter(n, Node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = api->getFrameFilter(n, Node, frameCtx);
        VSFrameRef* dst = api->newVideoFrame(VInfo->format, VInfo->width, VInfo->height, src, core);

        ProcessFrame(VpyFrame(src, api), VpyFrame(dst, api));

        api->freeFrame(src);
        return dst;
    }

    return nullptr;
}

void ContinuousMaskVpy::Free(VSCore* core, const VSAPI* api)
{
}

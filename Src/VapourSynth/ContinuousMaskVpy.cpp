#include "ContinuousMaskVpy.h"

void VS_CC ContinuousMaskVpy::Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api)
{
    VpyPropReader prop = VpyPropReader(api, in);
    VSNodeRef* src = prop.GetNode("clip");
    int radius = prop.GetInt("radius", 16);

	auto f = new ContinuousMaskVpy(in, out, src, core, api, radius);
    f->CreateFilter(in, out);
}

ContinuousMaskVpy::ContinuousMaskVpy(const VSMap* in, VSMap* out, VSNodeRef* node, VSCore* core, const VSAPI* api, int _radius) :
    VpyFilter(in, out, node, core, api),
	ContinuousMaskBase(new VpyVideo(node, api), VpyEnvironment(api, core), _radius)
{
}

void ContinuousMaskVpy::Init(VSMap* in, VSMap* out, VSNode* node)
{
}

VSFrameRef* ContinuousMaskVpy::GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx)
{
    if (activationReason == arInitial)
    {
        api->requestFrameFilter(n, Node, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = api->getFrameFilter(n, Node, frameCtx);
        VSFrameRef* dst = api->newVideoFrame(viSrc->format, viSrc->width, viSrc->height, src, core);

        ProcessFrame(VpyFrame(src, api), VpyFrame(dst, api));

        api->freeFrame(src);
        return dst;
    }

    return nullptr;
}

void ContinuousMaskVpy::Free()
{
}

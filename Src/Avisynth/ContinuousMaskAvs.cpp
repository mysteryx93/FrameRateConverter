#include "ContinuousMaskAvs.h"

ContinuousMask::ContinuousMask(PClip _child, int _radius, IScriptEnvironment* env) :
	GenericVideoFilter(_child), 
	ContinuousMaskBase(new AvsVideo(_child), AvsEnvironment(env), _radius)
{
}

PVideoFrame __stdcall ContinuousMask::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	ProcessFrame(AvsFrame(src, vi), AvsFrame(dst, vi));
	return dst;
}

// Marks filter as multi-threading friendly.
int __stdcall ContinuousMask::SetCacheHints(int cachehints, int frame_range)
{
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

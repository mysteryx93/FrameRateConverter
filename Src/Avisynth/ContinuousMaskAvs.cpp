#include "ContinuousMaskAvs.h"

AVSValue __cdecl ContinuousMaskAvs::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new ContinuousMaskAvs(args[0].AsClip(), args[1].AsInt(16), env);
}

ContinuousMaskAvs::ContinuousMaskAvs(PClip _child, int _radius, IScriptEnvironment* env) :
	GenericVideoFilter(_child), 
	ContinuousMaskBase(new AvsVideo(_child), AvsEnvironment(env), _radius)
{
}

PVideoFrame __stdcall ContinuousMaskAvs::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	ProcessFrame(AvsFrame(src, vi), AvsFrame(dst, vi));
	return dst;
}

// Marks filter as multi-threading friendly.
int __stdcall ContinuousMaskAvs::SetCacheHints(int cachehints, int frame_range)
{
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

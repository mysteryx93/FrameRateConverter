#include "StripeMask.h"

StripeMask::StripeMask(PClip _child, int _blksize, int _blksizev, int _overlap, int _overlapv, int _thr, int _comp, int _compv, int _str, int _strf, bool _lines, IScriptEnvironment* env) :
	GenericVideoFilter(_child), StripeMaskBase(AvsVideo(_child), _blksize, _blksizev, _overlap, _overlapv, _thr, _comp, _compv, _str, _strf, _lines, AvsEnvironment(env))
{
	int b = vi.BitsPerComponent();
	vi.pixel_type = b == 8 ? VideoInfo::CS_Y8 : b == 10 ? VideoInfo::CS_Y10 : b == 12 ? VideoInfo::CS_Y12 : b == 14 ? VideoInfo::CS_Y14 : b == 16 ? VideoInfo::CS_Y16 : b == 32 ? VideoInfo::CS_Y32 : VideoInfo::CS_Y8;
}

PVideoFrame __stdcall StripeMask::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame src2 = nullptr;
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (strf > 0 && n < vi.num_frames - 1)
	{
		PVideoFrame src2 = child->GetFrame(n + 1, env);
	}
	ProcessFrame(AvsFrame(src, vi), AvsFrame(src2, vi), AvsFrame(dst, vi), AvsEnvironment(env));
	return dst;
}

// Marks filter as multi-threading friendly.
int __stdcall StripeMask::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

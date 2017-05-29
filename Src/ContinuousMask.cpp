#include "ContinuousMask.h"

ContinuousMask::ContinuousMask(PClip _child, int _radius, IScriptEnvironment* env) :
	GenericVideoFilter(_child), radius(_radius) {

	if (!vi.IsYUV() && !vi.IsY())
	if (radius <= 1)
		env->ThrowError("ContinuousMask: Radius must be above 1");
}

ContinuousMask::~ContinuousMask() {
}

PVideoFrame __stdcall ContinuousMask::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (vi.BitsPerComponent() == 8)
		Calculate<short, BYTE>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	else if (vi.BitsPerComponent() < 32)
		Calculate<int, short>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	else
		Calculate<float, float>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	return dst;
}

template<typename T, typename P> void ContinuousMask::Calculate(const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch, IScriptEnvironment* env) {
	memset(dstp, 0, dstPitch * vi.height);
	T Sum = 0;
	const P* srcIter = (const P*)srcp;
	P* dstIter = (P*)dstp;
	short radFwd, radBck;
	short radFwdV, radBckV;
	srcPitch = srcPitch / sizeof(P);
	dstPitch = dstPitch / sizeof(P);

	// Calculate the average of [radius] pixels in all 4 directions, for source pixels having a value.
	for (int y = 0; y < vi.height; y++) {
		for (int x = 0; x < vi.width; x++) {
			if (srcIter[x] > 0) {
				Sum = 0;
				radFwd = min(radius, vi.width - x);
				radBck = min(min(radius, x + 1), vi.width) - 1;
				radFwdV = min(radius, vi.height - y);
				radBckV = min(min(radius, y + 1), vi.height) - 1;
				for (short i = -radBck; i < radFwd; i++) {
					Sum += (T)srcIter[x + i];
				}
				for (short i = -radBckV; i < radFwdV; i++) {
					Sum += (T)srcIter[x + i * srcPitch];
				}
				dstIter[x] = P(Sum / (radFwd + radBck + radFwdV + radBckV));
			}
		}
		srcIter += srcPitch;
		dstIter += dstPitch;
	}
}

// Marks filter as multi-threading friendly.
int __stdcall ContinuousMask::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

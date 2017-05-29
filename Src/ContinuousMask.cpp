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
	const BYTE* srcp = src->GetReadPtr();
	int srcPitch = src->GetPitch();

	PVideoFrame dst = env->NewVideoFrame(vi);
	BYTE* dstp = dst->GetWritePtr();
	const int dstPitch = dst->GetPitch();
	memset(dstp, 0, dstPitch * vi.height);

	short Sum = 0;
	const BYTE* srcIter = srcp;
	BYTE* dstIter = dstp;
	short radFwd, radBck;
	short radFwdV, radBckV;

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
					Sum += srcIter[x + i];
				}
				for (short i = -radBckV; i < radFwdV; i++) {
					Sum += srcIter[x + i * srcPitch];
				}
				dstIter[x] = BYTE(Sum / (radFwd + radBck + radFwdV + radBckV));
			}
		}
		srcIter += srcPitch;
		dstIter += dstPitch;
	}

	return dst;
}

// Marks filter as multi-threading friendly.
int __stdcall ContinuousMask::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

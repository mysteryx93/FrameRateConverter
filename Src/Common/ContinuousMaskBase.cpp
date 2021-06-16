#include "ContinuousMaskBase.h"

ContinuousMaskBase::ContinuousMaskBase(ICommonVideo* _clip, ICommonEnvironment& _env, int _radius) :
	width(_clip->Width()), height(_clip->Height()), radius(_radius), bitsPerSample(_clip->BitsPerSample()), env(_env)
{
	if (!_clip->IsYUV() && !_clip->IsY())
		env.ThrowError("ContinuousMask: Clip must be in Y or YUV format");
	if (radius <= 1)
		env.ThrowError("ContinuousMask: Radius must be above 1");
}

void ContinuousMaskBase::ProcessFrame(ICommonFrame& src, ICommonFrame& dst)
{
	if (bitsPerSample == 8)
		Calculate<uint16_t, BYTE>(src.GetReadPtr(), src.GetStride(), dst.GetWritePtr(), dst.GetStride());
	else if (bitsPerSample < 32)
		Calculate<uint32_t, uint16_t>(src.GetReadPtr(), src.GetStride(), dst.GetWritePtr(), dst.GetStride());
	else
		Calculate<float, float>(src.GetReadPtr(), src.GetStride(), dst.GetWritePtr(), dst.GetStride());
}

// T: data type to calculate total (must hold P.MaxValue * radius * 4)
// P: data type of each pixel
template<typename T, typename P> void ContinuousMaskBase::Calculate(const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch)
{
	memset(dstp, 0, static_cast<size_t>(dstPitch) * height);
	T Sum = 0;
	const P* srcIter = (const P*)srcp;
	P* dstIter = (P*)dstp;
	int radFwd, radBck;
	int radFwdV, radBckV;
	srcPitch = srcPitch / sizeof(P);
	dstPitch = dstPitch / sizeof(P);

	// Calculate the average of [radius] pixels in all 4 directions, for source pixels having a value.
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (srcIter[x] > 0) {
				Sum = 0;
				radFwd = min(radius, width - x);
				radBck = min(min(radius, x + 1), width) - 1;
				radFwdV = min(radius, height - y);
				radBckV = min(min(radius, y + 1), height) - 1;
				for (int i = -radBck; i < radFwd; i++) {
					Sum += (T)srcIter[x + i];
				}
				for (int i = -radBckV; i < radFwdV; i++) {
					Sum += (T)srcIter[x + i * srcPitch];
				}
				dstIter[x] = P(Sum / (radFwd + radBck + radFwdV + radBckV));
			}
		}
		srcIter += srcPitch;
		dstIter += dstPitch;
	}
}

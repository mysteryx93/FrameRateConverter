#include "ContinuousMaskBase.h"

const char* ContinuousMaskBase::PluginName = "ContinuousMask";

ContinuousMaskBase::ContinuousMaskBase(ICommonVideo* _child, ICommonEnvironment& _env, int _radius, int _thr) :
	source(_child), radius(_radius), thr(_thr), bitsPerSample(_child->BitsPerSample()), env(_env)
{
	if (radius <= 1)
	{
		env.ThrowError("radius must be above 1");
	}
	else if (thr < 0 || thr > 255)
	{
		env.ThrowError("thr must be between 0 and 255.");
	}
}

void ContinuousMaskBase::ProcessFrame(ICommonFrame& src, ICommonFrame& dst)
{
	for (int i = 0; i < source->NumPlanes(); i++)
	{
		auto width = src.GetWidth(i);
		auto height = src.GetHeight(i);
		auto srcp = src.GetReadPtr(i);
		auto srcPitch = src.GetStride(i);
		auto dstp = dst.GetWritePtr(i);
		auto dstPitch = dst.GetStride(i);

		if (bitsPerSample == 8)
		{
			Calculate<uint16_t, BYTE>(width, height, srcp, srcPitch, dstp, dstPitch);
		}
		else if (bitsPerSample < 32)
		{
			Calculate<uint32_t, uint16_t>(width, height, srcp, srcPitch, dstp, dstPitch);
		}
		else
		{
			Calculate<float, float>(width, height, srcp, srcPitch, dstp, dstPitch);
		}
	}
}

// T: data type to calculate total (must hold P.MaxValue * radius * 4)
// P: data type of each pixel
template<typename T, typename P> void ContinuousMaskBase::Calculate(int width, int height, const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch)
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
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (srcIter[x] > thr)
			{
				Sum = 0;
				radFwd = min(radius, width - x);
				radBck = min(min(radius, x + 1), width) - 1;
				radFwdV = min(radius, height - y);
				radBckV = min(min(radius, y + 1), height) - 1;
				for (int i = -radBck; i < radFwd; i++)
				{
					Sum += (T)srcIter[x + i];
				}
				for (int i = -radBckV; i < radFwdV; i++)
				{
					Sum += (T)srcIter[x + i * srcPitch];
				}
				dstIter[x] = P(Sum / (radFwd + radBck + radFwdV + radBckV));
			}
		}
		srcIter += srcPitch;
		dstIter += dstPitch;
	}
}

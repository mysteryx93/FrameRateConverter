#include "ConvertFpsLimitBase.h"

ConvertFPSLimitBase::ConvertFPSLimitBase(ICommonVideo* _child, unsigned new_numerator, unsigned new_denominator, int _ratio, ICommonEnvironment& env)
	: source(_child), ratio(_ratio)
{
	if (_ratio < 0 || _ratio > 100)
		env.ThrowError("ConvertFpsLimit: ratio must be between 0 (frame copy) and 100 (full blend)");

	fa = int64_t(source->FpsNum()) * new_denominator;
	fb = int64_t(source->FpsDen()) * new_numerator;
	if (3 * fb < (fa << 1))
	{
		int dec = MulDiv(source->FpsNum(), 20000, source->FpsDen());
		env.ThrowErrorFormat("ConvertFPS: New frame rate too small. Must be greater than %d.%04d ", dec / 30000, (dec / 3) % 10000);
	}
}

ICommonFrame& ConvertFPSLimitBase::ProcessFrame(int n, ICommonFrame& src, ICommonFrame& srcNext, ICommonEnvironment& env)
{
	static const int resolution = 10; //bits. Must be >= 4, or modify next line
	static const int threshold = (1 << (resolution - 4)) * ratio / 100;
	static const int one = 1 << resolution;
	static const int half = 1 << (resolution - 1);

	//double nsrc_f, frac_f;
	//frac_f = modf((double)n * fa / fb, &nsrc_f);
	// integer versions
	int nsrc = int(n * fa / fb);
	int frac = int((((n * fa) % fb) << resolution) / fb);

	double frac_f_from_int = (double)frac / one;

	// Mode 1: Blend full frames
	int mix_ratio = frac;

	if (mix_ratio < half)
		mix_ratio = mix_ratio * ratio / 100;
	else
		mix_ratio = one - ((one - mix_ratio) * ratio / 100);

	// Don't bother if the blend ratio is small
	if (mix_ratio < threshold)
		return src;

	if (mix_ratio > (one - threshold))
		return srcNext;

	float mix_ratio_f = (float)mix_ratio / one;

	env.MakeWritable(src);

	//const int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
	//const int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
	const int planes[4] = { 0, 1, 2, 3 };

	//int planeCount;
	int planeCount = source->NumPlanes();
	//planes = (!vinfo->IsPlanar() || vinfo->IsYUV()) ? planes_y : planes_r;

	const int bits_per_pixel = source->BitsPerSample();
	for (int plane = 0; plane < planeCount; ++plane)
	{
		// const int plane = planes[j];
		const BYTE* b_data = srcNext.GetReadPtr(plane);
		int          b_pitch = srcNext.GetStride(plane);
		BYTE* a_data = src.GetWritePtr(plane);
		int          a_pitch = src.GetStride(plane);
		int          row_size = src.GetRowSize(plane);
		int          height = src.GetHeight(plane);

		int weight_i;
		int invweight_i;
		// float weight = (float)frac_f;
		MergeFuncPtr weighted_merge_planar = getMergeFunc(bits_per_pixel, env.GetCpuSupport(), a_data, b_data, mix_ratio_f, /*out*/weight_i, /*out*/invweight_i);
		weighted_merge_planar(a_data, b_data, a_pitch, b_pitch, row_size, height, mix_ratio_f, weight_i, invweight_i);
	}

	return src;
}

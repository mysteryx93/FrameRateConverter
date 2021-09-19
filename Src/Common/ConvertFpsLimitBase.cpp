#include "ConvertFpsLimitBase.h"

const char* ConvertFPSLimitBase::PluginName = "ConvertFpsLimit";

ConvertFPSLimitBase::ConvertFPSLimitBase(ICommonVideo* _child, ICommonEnvironment& env, int new_numerator, int new_denominator, int _ratio)
	: source(_child), ratio(_ratio), fa(0), fb(0)
{
	if (_ratio < 0 || _ratio > 100)
	{
		env.ThrowError("ratio must be between 0 (frame copy) and 100 (full blend)");
		return;
	}

	fa = int64_t(source->FpsNum()) * new_denominator;
	fb = int64_t(source->FpsDen()) * new_numerator;
	//if (3 * fb < (fa << 1))
	//{
	//	int dec = MulDiv(source->FpsNum(), 20000, source->FpsDen());
	//	env.ThrowError("New frame rate too small. Must be greater than %d.%04d ", dec / 30000, (dec / 3) % 10000);
	//	return;
	//}
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





/*********************************************
 ******    Float and Rational utility   ******
 *********************************************

 *********************************************
 ******  Thanks to RAYMOD2 for his help  *****
 ******  in beating continued fractions  *****
 ******  into a usable form.       IanB  *****
 *********************************************/

 // This function converts a 32-bit IEEE float into a fraction.  This
 // process is lossless but it may fail for very large or very small
 // floats.  It also discards the sign bit.  Since the denominator will
 // always be a power of 2 and the numerator will always be odd (except
 // when the denominator is 1) the fraction will already be reduced
 // to its lowest terms. output range (2^32-2^(32-24))/1 -- (1/2^31)
 // i.e. 4294967040/1 -- 1/2147483648 (4.65661287307e-10)
 // returns true if input is out of range

bool ConvertFPSLimitBase::float_to_frac(float input, unsigned& num, unsigned& den)
{
	union { float f; unsigned i; } value{};
	uint32_t mantissa;
	int exponent;

	// Bit strip the float
	value.f = input;
	mantissa = (value.i & 0x7FFFFF) + 0x800000;  // add implicit bit on the left
	exponent = ((value.i & 0x7F800000) >> 23) - 127 - 23;  // remove decimal pt

	// minimize the mantissa by removing any trailing 0's
	while (!(mantissa & 1)) { mantissa >>= 1; exponent += 1; }

	// if too small try to pull result from the reciprocal
	if (exponent < -31) {
		return float_to_frac((float)(1.0 / input), den, num);
	}

	// if exponent is too large try removing leading 0's of mantissa
	while ((exponent > 0) && !(mantissa & 0x80000000)) {
		mantissa <<= 1; exponent -= 1;
	}
	if (exponent > 0) {  // number too large
		num = 0xffffffff;
		den = 1;
		return true; // Out of range!
	}
	num = mantissa;
	den = 1 << (-exponent);

	return false;
}


// This function uses continued fractions to find the rational
// approximation such that the result truncated to a float is
// equal to value. The semiconvergents for the smallest such
// rational pair is then returned. The algorithm is modified
// from Wikipedia, Continued Fractions.

bool ConvertFPSLimitBase::reduce_float(float value, unsigned& num, unsigned& den)
{
	if (float_to_frac(value, num, den)) {
		return true;
	}

	uint32_t n0 = 0, n1 = 1, n2, nx = num;  // numerators
	uint32_t d0 = 1, d1 = 0, d2, dx = den;  // denominators
	uint32_t a2, ax, amin;  // integer parts of quotients
	uint32_t f1 = 0, f2;    // fractional parts of quotients

	for (;;)  // calculate convergents
	{
		a2 = nx / dx;
		f2 = nx % dx;
		n2 = n0 + n1 * a2;
		d2 = d0 + d1 * a2;

		if (f2 == 0) break;  // no more convergents (n2 / d2 == input)

		// Damn compiler does not correctly cast
		// to intermediate results to float
		float f = (float)((double)n2 / d2);
		if (f == value) break;

		n0 = n1; n1 = n2;
		d0 = d1; d1 = d2;
		nx = dx; dx = f1 = f2;
	}
	if (d2 == 1)
	{
		num = n2;
		den = d2;
	}
	else { // we have been through the loop at least twice

		if ((a2 % 2 == 0) && (d0 * f1 > f2 * d1))
			amin = a2 / 2;  // passed 1/2 a_k admissibility test
		else
			amin = a2 / 2 + 1;

		// find the sign of the error (actual + error = n2/d2) and then
		// set (n2/d2) = (num/den + sign * epsilon) = R2 and solve for ax
		//--------------------
		//   n2 = n0 + n1 * ax
		//   d2 = d0 + d1 * ax
		//-----------------------------------------------
		//   (n2/d2)       = R2     = (n0 + n1 * ax)/(d0 + d1 * ax)
		//   n0 + n1 * ax           = R2 * (d0 + d1 * ax)
		//   n0 + n1 * ax           = R2 * d0 + R2 * d1 * ax
		//   R2 * d1 * ax - n1 * ax = n0 - R2 * d0
		//   (R2 * d1 - n1) * ax    = n0 - R2 * d0
		//   ax                     = (n0 - R2 * d0)/(R2 * d1 - n1)

			// bump float to adjacent float value
		union { float f; uint32_t i; } eps{}; eps.f = value;
		if (UInt32x32To64(n1, den) > UInt32x32To64(num, d1)) {
			eps.i -= 1;
		}
		else {
			eps.i += 1;
		}
		double r2 = eps.f;
		r2 += value;
		r2 /= 2;

		double yn = n0 - r2 * d0;
		double yd = r2 * d1 - n1;
		ax = (uint32_t)((yn + yd) / yd); // ceiling value

		if (ax < amin) ax = amin;

		// calculate nicest semiconvergent
		num = n0 + n1 * ax;
		den = d0 + d1 * ax;
	}
	return false;
}


// This function uses continued fractions to find the best rational
// approximation that satisfies (denom <= limit).  The algorithm
// is from Wikipedia, Continued Fractions.
//
void ConvertFPSLimitBase::reduce_frac(uint32_t& num, uint32_t& den, uint32_t limit)
{
	uint32_t n0 = 0, n1 = 1, n2, nx = num;    // numerators
	uint32_t d0 = 1, d1 = 0, d2, dx = den;  // denominators
	uint32_t a2, ax, amin;  // integer parts of quotients
	uint32_t f1 = 0, f2;        // fractional parts of quotients
	int i = 0;  // number of loop iterations

	for (;;) { // calculate convergents
		a2 = nx / dx;
		f2 = nx % dx;
		n2 = n0 + n1 * a2;
		d2 = d0 + d1 * a2;

		if (f2 == 0) break;
		if ((i++) && (d2 >= limit)) break;

		n0 = n1; n1 = n2;
		d0 = d1; d1 = d2;
		nx = dx; dx = f1 = f2;
	}
	if (d2 <= limit)
	{
		num = n2; den = d2;  // use last convergent
	}
	else { // (d2 > limit)
	  // d2 = d0 + d1 * ax
	  // d1 * ax = d2 - d1
		ax = (limit - d0) / d1;  // set d2 = limit and solve for a2

		if ((a2 % 2 == 0) && (d0 * f1 > f2 * d1))
			amin = a2 / 2;  // passed 1/2 a_k admissibility test
		else
			amin = a2 / 2 + 1;

		if (ax < amin) {
			// use previous convergent
			num = n1;
			den = d1;
		}
		else {
			// calculate best semiconvergent
			num = n0 + n1 * ax;
			den = d0 + d1 * ax;
		}
	}
}


/***************************************
 *******   Float to FPS utility   ******
 ***************************************/

void ConvertFPSLimitBase::FloatToFPS(float n, uint32_t& num, uint32_t& den, ICommonEnvironment& env)
{
	if (n <= 0)
		env.ThrowError("FPS must be greater then 0.");

	float x;
	uint32_t u = (uint32_t)(n * 1001 + 0.5);

	// Check for the 30000/1001 multiples
	x = (float)((u / 30000.0 * 30000) / 1001.0);
	if (x == n) { num = u; den = 1001; return; }

	// Check for the 24000/1001 multiples
	x = (float)((u / 24000.0 * 24000) / 1001.0);
	if (x == n) { num = u; den = 1001; return; }

	if (n < 14.986) {
		// Check for the 30000/1001 factors
		u = (uint32_t)(30000.0 / n + 0.5);
		x = (float)(30000.0 / (u / 1001.0 * 1001));
		if (x == n) { num = 30000; den = u; return; }

		// Check for the 24000/1001 factors
		u = (uint32_t)(24000.0 / n + 0.5);
		x = (float)(24000.0 / (u / 1001.0 * 1001));
		if (x == n) { num = 24000; den = u; return; }
	}

	// Find the rational pair with the smallest denominator
	// that is equal to n within the accuracy of an IEEE float.
	if (reduce_float(n, num, den))
		env.ThrowError("FPS value is out of range.");

}


/****************************************
 *******   Preset to FPS utility   ****** -- Tritcal, IanB Jan 2006
 ****************************************/

void ConvertFPSLimitBase::PresetToFPS(const char* p, uint32_t& num, uint32_t& den, ICommonEnvironment& env)
{
	if (lstrcmpi(p, "ntsc_film") == 0) { num = 24000; den = 1001; }
	else if (lstrcmpi(p, "ntsc_video") == 0) { num = 30000; den = 1001; }
	else if (lstrcmpi(p, "ntsc_double") == 0) { num = 60000; den = 1001; }
	else if (lstrcmpi(p, "ntsc_quad") == 0) { num = 120000; den = 1001; }

	else if (lstrcmpi(p, "ntsc_round_film") == 0) { num = 2997; den = 125; }
	else if (lstrcmpi(p, "ntsc_round_video") == 0) { num = 2997; den = 100; }
	else if (lstrcmpi(p, "ntsc_round_double") == 0) { num = 2997; den = 50; }
	else if (lstrcmpi(p, "ntsc_round_quad") == 0) { num = 2997; den = 25; }

	else if (lstrcmpi(p, "film") == 0) { num = 24; den = 1; }

	else if (lstrcmpi(p, "pal_film") == 0) { num = 25; den = 1; }
	else if (lstrcmpi(p, "pal_video") == 0) { num = 25; den = 1; }
	else if (lstrcmpi(p, "pal_double") == 0) { num = 50; den = 1; }
	else if (lstrcmpi(p, "pal_quad") == 0) { num = 100; den = 1; }

	else if (lstrcmpi(p, "drop24") == 0) { num = 24000; den = 1001; }
	else if (lstrcmpi(p, "drop30") == 0) { num = 30000; den = 1001; }
	else if (lstrcmpi(p, "drop60") == 0) { num = 60000; den = 1001; }
	else if (lstrcmpi(p, "drop120") == 0) { num = 120000; den = 1001; }
	/*
		else if (lstrcmpi(p, "drop25"           ) == 0) { num = 25000; den = 1001; }
		else if (lstrcmpi(p, "drop50"           ) == 0) { num = 50000; den = 1001; }
		else if (lstrcmpi(p, "drop100"          ) == 0) { num =100000; den = 1001; }

		else if (lstrcmpi(p, "nondrop24"        ) == 0) { num =    24; den =    1; }
		else if (lstrcmpi(p, "nondrop25"        ) == 0) { num =    25; den =    1; }
		else if (lstrcmpi(p, "nondrop30"        ) == 0) { num =    30; den =    1; }
		else if (lstrcmpi(p, "nondrop50"        ) == 0) { num =    50; den =    1; }
		else if (lstrcmpi(p, "nondrop60"        ) == 0) { num =    60; den =    1; }
		else if (lstrcmpi(p, "nondrop100"       ) == 0) { num =   100; den =    1; }
		else if (lstrcmpi(p, "nondrop120"       ) == 0) { num =   120; den =    1; }

		else if (lstrcmpi(p, "23.976"           ) == 0) { num = 24000; den = 1001; }
		else if (lstrcmpi(p, "23.976!"          ) == 0) { num =  2997; den =  125; }
		else if (lstrcmpi(p, "24.0"             ) == 0) { num =    24; den =    1; }
		else if (lstrcmpi(p, "25.0"             ) == 0) { num =    25; den =    1; }
		else if (lstrcmpi(p, "29.97"            ) == 0) { num = 30000; den = 1001; }
		else if (lstrcmpi(p, "29.97!"           ) == 0) { num =  2997; den =  100; }
		else if (lstrcmpi(p, "30.0"             ) == 0) { num =    30; den =    1; }
		else if (lstrcmpi(p, "59.94"            ) == 0) { num = 60000; den = 1001; }
		else if (lstrcmpi(p, "59.94!"           ) == 0) { num =  2997; den =   50; }
		else if (lstrcmpi(p, "60.0"             ) == 0) { num =    60; den =    1; }
		else if (lstrcmpi(p, "100.0"            ) == 0) { num =   100; den =    1; }
		else if (lstrcmpi(p, "119.88"           ) == 0) { num =120000; den = 1001; }
		else if (lstrcmpi(p, "119.88!"          ) == 0) { num =  2997; den =   25; }
		else if (lstrcmpi(p, "120.0"            ) == 0) { num =   120; den =    1; }
	*/
	else {
		env.ThrowError("Invalid preset value used.");
	}
}

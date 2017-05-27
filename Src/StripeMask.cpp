#include "StripeMask.h"

StripeMask::StripeMask(PClip _child, int _blksize, int _blksizev, int _overlap, int _overlapv, int _trh, int _comp, int _compv, int _str, int _strf, bool _lines, IScriptEnvironment* env) :
	GenericVideoFilter(_child), blksize(_blksize), blksizev(_blksizev), overlap(_overlap), overlapv(_overlapv), trh(_trh), comp(_comp), compv(_compv), str(_str), strf(_strf), lines(_lines) {

	if (!vi.IsYUV() && !vi.IsY())
		env->ThrowError("StripeMask: clip must be Y or YUV format");
	if (blksize < 0)
		env->ThrowError("StripeMask: blksize must be 0 or above");
	if (blksizev < 0)
		env->ThrowError("StripeMask: blksizev must be 0 or above");
	if (blksize != 0 && (overlap < 0 || overlap >= blksize))
		env->ThrowError("StripeMask: overlap must be smaller than blksize");
	if (blksizev != 0 && (overlapv < 0 || overlapv >= blksizev))
		env->ThrowError("StripeMask: overlapv must be smaller than blksizev");
	if (trh < 0 || trh > 255)
		env->ThrowError("StripeMask: Trh must be between 0 and 255");
	if (comp < 2 || comp > 5)
		env->ThrowError("StripeMask: Comp must be between 2 and 5");
	if (compv < 2 || compv > 5)
		env->ThrowError("StripeMask: CompV must be between 2 and 5");
	if (str < 0 || str > 255)
		env->ThrowError("StripeMask: Str must be between 0 and 255");
	if (strf < 0 || strf > 255)
		env->ThrowError("StripeMask: StrF must be between 0 and 255");

	int b = vi.BitsPerComponent();
	vi.pixel_type = b == 8 ? VideoInfo::CS_Y8 : b == 10 ? VideoInfo::CS_Y10 : b == 12 ? VideoInfo::CS_Y12 : b == 14 ? VideoInfo::CS_Y14 : b == 16 ? VideoInfo::CS_Y16 : b == 32 ? VideoInfo::CS_Y32 : VideoInfo::CS_Y8;
}

StripeMask::~StripeMask() {
}

PVideoFrame __stdcall StripeMask::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame src = child->GetFrame(n, env);
	const BYTE* srcp = src->GetReadPtr();
	int srcPitch = src->GetPitch();

	PVideoFrame dst = env->NewVideoFrame(vi);
	BYTE* dstp = dst->GetWritePtr();
	const int dstPitch = dst->GetPitch();
	memset(dstp, 0, dstPitch * vi.height);

	// Create memory buffers.
	BYTE* LineAvgH = new BYTE[vi.width];
	BYTE* LineAvgV = new BYTE[vi.height]; // may need to size mod 16
	PatternStep* HistoryH = new PatternStep[vi.width + 2]; // +2 to store initial data to compare to
	PatternStep* HistoryV = new PatternStep[vi.height + 2];

	// Add next frame with 50% transparency.
	if (strf > 0 && n < vi.num_frames - 1) {
		PVideoFrame src2 = child->GetFrame(n + 1, env);
		const BYTE* srcp2 = src2->GetReadPtr();
		int srcPitch2 = src2->GetPitch();

		if (blksize > 0)
			CalcFrame(srcp2, srcPitch2, dstp, dstPitch, LineAvgH, HistoryH, strf, false);
		if (blksizev > 0)
			CalcFrame(srcp2, srcPitch2, dstp, dstPitch, LineAvgV, HistoryV, strf, true);
	}

	// Calculte current frame.
	if (blksize > 0)
		CalcFrame(srcp, srcPitch, dstp, dstPitch, LineAvgH, HistoryH, str, false);
	if (blksizev > 0)
		CalcFrame(srcp, srcPitch, dstp, dstPitch, LineAvgV, HistoryV, str, true);

	delete LineAvgH;
	delete LineAvgV;
	delete HistoryH;
	delete HistoryV;

	return dst;
}

void StripeMask::CalcFrame(const BYTE* src, int srcPitch, BYTE* dst, int dstPitch, BYTE* lineAvg, PatternStep* history, float strength, bool vertical) {
	int blk = !vertical ? blksize : blksizev;
	int over = !vertical ? overlap : overlapv;
	int width = !vertical ? vi.width : vi.height;
	int height = !vertical ? vi.height : vi.width;
	int compBck = (!vertical ? comp : compv) / 2;
	int compFwd = ((!vertical ? comp : compv) - 1) / 2;

	int i = 0;
	int offset;
	while (i < height - over) {
		if (i >= height - blk)
			i = height - blk;
		CalcBandAvg(src + (!vertical ? srcPitch * i : i), srcPitch, width, lineAvg, blk, vertical);
		CalcBand(dst + (!vertical ? dstPitch * i : i), dstPitch, width, lineAvg, history, blk, strength, vertical, compBck, compFwd);
		i += blk - over;
	}
}

// Calculates a band with the average value of each bar on the band.
// The result will be stored in lineAvg which must be defined with the right size.
// Allows calculating both horizontal and vertical bands.
void StripeMask::CalcBandAvg(const BYTE* src, int pitch, int size, BYTE* lineAvg, int blk, bool vertical) {
	int Sum;
	if (!vertical) {
		for (int i = 0; i < size; i++) {
			Sum = 0;
			for (int j = 0; j < blk; j++) {
				Sum += src[pitch*j];
			}
			lineAvg[i] = Sum / blk;
			src++;
		}
	}
	else {
		for (int i = 0; i < size; i++) {
			Sum = 0;
			for (int j = 0; j < blk; j++) {
				Sum += src[j];
			}
			lineAvg[i] = Sum / blk;
			src += pitch;
		}
	}
}

// Lightens the mask with the contrast between values.
// From the contrast lines, find and mark regular patterns
void StripeMask::CalcBand(BYTE* dst, int dstPitch, int size, BYTE* lineAvg, PatternStep* history, int blk, float strength, bool vertical, int compFwd, int compBck) {
	BYTE ValDif;
	// Alternate between 'on' and 'off' after binarizing contrasts, then scan for alternating states patterns.
	bool Alt = false;
	PatternStep AltVal;
	int AltPos = 0;
	history[0] = PatternStep(0, 0);
	history[1] = PatternStep(0, 0);
	int hLength = 2;
	int PatternStart = -1;
	int PatternEnd = -1;
	int PatternLength = 0; // 0=no pattern, 2=single-stripe pattern, 4=double-stripe pattern
	int PatternCont = 0;

	for (int i = 0; i < size; i++) {
		ValDif = GetDiff(lineAvg, i, size, compFwd, compBck);

		if (lines) {
			// Mark line contrasts.
			if (ValDif >= trh)
				MarkArea(dst, dstPitch, i, i + 1, strength, blk, vertical);
		}
		else {
			if ((ValDif >= trh && !Alt) || (ValDif < trh && Alt)) {
				// Build history of alternating states.
				Alt = !Alt;
				history[hLength++] = PatternStep(i, i - AltPos);
				AltPos = i;

				if (!Alt) {
					// Look through history to find patterns.
					if (PatternLength == 0) {
						// 2 last alt match 2 previous ones, or 4 last alt match 4 previous ones.
						if (hLength >= 8)
							PatternLength = CompareHistory(history, hLength, 4, blk) ? 4 : 0;
						if (PatternLength == 0 && hLength >= 4)
							PatternLength = CompareHistory(history, hLength, 2, blk) ? 2 : 0;
						if (PatternLength > 0)
							PatternStart = history[max(0, hLength - PatternLength * 2 - 2)].Pos;
					}
					else if (PatternLength > 0) {
						// Once pattern is detected, detect when pattern breaks.
						if (CompareHistory(history, hLength, PatternLength, blk)) {
							PatternCont += 2;
						}
						else {
							PatternEnd = history[hLength - 3].Pos;
							PatternLength = 0;
							PatternCont = 0;
							if (hLength > 6) // if hLength=6, 0-1 is blank, 2-3 is 'pattern start' and 4-5 is pattern break.
								MarkArea(dst, dstPitch, PatternStart, PatternEnd, strength, blk, vertical);
						}
					}
					// If 2 full blocks are continuous white, mark it.
					if (history[hLength - 1].Val > blk * 2) {
						MarkArea(dst, dstPitch, history[hLength - 2].Pos, i, strength, blk, vertical);
					}
				}
			}
		}
	}

	if (!lines && PatternLength > 0)
		MarkArea(dst, dstPitch, PatternStart, history[hLength - 1].Pos, strength, blk, vertical);
}

// Calculate contrast between values
// We take the difference between the max and min of several values.
int StripeMask::GetDiff(BYTE* lineAvg, int n, int size, int compBck, int compFwd) {
	if (n >= compBck && n < size - compFwd) {
		BYTE ValMin = lineAvg[n - compBck];
		BYTE ValMax = ValMin;
		for (int i = -compBck + 1; i <= compFwd; i++) {
			ValMin = min(ValMin, lineAvg[n + i]);
			ValMax = max(ValMax, lineAvg[n + i]);
		}
		return ValMax - ValMin;
	}
	else
		return 0;
}

void StripeMask::MarkArea(BYTE* dst, int dstPitch, int patternStart, int patternEnd, BYTE strength, int blk, bool vertical) {
	if (!vertical) {
		for (int x = patternStart; x < patternEnd; x++) {
			for (int y = 0; y < blk; y++) {
				dst[x + dstPitch * y] = strength;
			}
		}
	}
	else {
		for (int x = patternStart; x < patternEnd; x++) {
			for (int y = 0; y < blk; y++) {
				dst[x * dstPitch + y] = strength;
			}
		}
	}
}

bool StripeMask::CompareHistory(PatternStep* history, int hLength, int length, int blk) {
	PatternStep* Item1 = &history[hLength - 1];
	PatternStep* Item2 = Item1 - length;
	for (int i = 0; i < length; i++) {
		// Maximum pattern step is blksize*2
		if (Item1->Val > blk*2 || Item1->Val > blk*2)
			return false;
		else if (abs(Item1->Val - Item2->Val) > (Item2->Val <= 4 ? 1 : Item2->Val <= 8 ? 2 : Item2->Val <= 16 ? 3 : 4))
			return false;
		Item1--;
		Item2--;
	}
	return true;
}

// Marks filter as multi-threading friendly.
int __stdcall StripeMask::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

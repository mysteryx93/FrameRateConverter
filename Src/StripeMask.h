#include "avisynth.h"
#include "avs\minmax.h"
#include <math.h>
#include <cstring>

struct PatternStep {
public:
	PatternStep() {};
	PatternStep(int _pos, int _val) : Pos(_pos), Val(_val) {};
	int Pos;
	int Val;
};

enum MaskMode {
	Lines,
	Patterns
};

class StripeMask : public GenericVideoFilter {
public:
	StripeMask(PClip _child, int _blksize, int _blksizev, int _overlap, int _overlapv, int _trh, int comp, int compv, int _str, int _strf, bool _lines, IScriptEnvironment* env);
	~StripeMask();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	int __stdcall SetCacheHints(int cachehints, int frame_range);
private:
	void CalcFrame(const BYTE* src, int srcPitch, BYTE* dst, int dstPitch, BYTE* lineAvg, PatternStep* history, float strength, bool vertical);
	void CalcBandAvg(const BYTE* src, int pitch, int width, BYTE* lineAvg, int blk, bool vertical);
	void CalcBand(BYTE* dst, int dstPitch, int size, BYTE* lineAvg, PatternStep* history, int blk, float strength, bool vertical, int compFwd, int compBck);
	int GetDiff(BYTE* lineAvg, int n, int size, int compBck, int compFwd);
	void MarkArea(BYTE* dst, int dstPitch, int patternStart, int patternEnd, BYTE strength, int blk, bool vertical);
	bool CompareHistory(PatternStep* history, int historySize, int length, int blk);

	const int blksize;
	const int blksizev;
	const int overlap;
	const int overlapv;
	const int trh;
	const int comp;
	const int compv;
	const int str;
	const int strf;
	const bool lines;
	BYTE* dstStart;
	BYTE* dstEnd;
};

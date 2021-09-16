#include "../Environments/Common.h"
#include <math.h>
#include <cstring>
#include <Windows.h>

struct PatternStep {
	PatternStep() {};
	PatternStep(int _pos, int _val) : Pos(_pos), Length(_val) {};
	int Pos = 0;
	int Length = 0;
};

enum MaskMode {
	Lines,
	Patterns
};

class StripeMaskBase {
public:
	static const char* PluginName;

protected:
	StripeMaskBase(ICommonVideo* _child, ICommonEnvironment& env, int _blksize, int _blksizev, int _overlap, int _overlapv, int _thr, int _comp, int _compv, int _str, bool _lines);
	~StripeMaskBase();
	void ProcessFrame(ICommonFrame& src, ICommonFrame& dst, ICommonEnvironment& env);

private:
	void CalcFrame(const BYTE* src, int srcPitch, BYTE* dst, int dstPitch, BYTE* lineAvg, PatternStep* history, BYTE strength, bool vertical);
	void CalcBandAvg(const BYTE* src, int pitch, int width, BYTE* lineAvg, int blk, bool vertical);
	void CalcBand(BYTE* dst, int dstPitch, int size, BYTE* lineAvg, PatternStep* history, int blk, BYTE strength, bool vertical, int compFwd, int compBck);
	int GetDiff(BYTE* lineAvg, int n, int size, int compBck, int compFwd);
	void MarkArea(BYTE* dst, int dstPitch, int patternStart, int patternEnd, BYTE strength, int blk, bool vertical);
	bool CompareHistory(PatternStep* history, int historySize, int length, int blk);

protected:
	ICommonVideo* source;
	const int blksize;
	const int blksizev;
	const int overlap;
	const int overlapv;
	const int thr;
	const int comp;
	const int compv;
	const int str;
	const bool lines;
};

#include "../Environments/Avisynth.hpp"
#include "../Common/StripeMaskBase.h"

class StripeMask : public GenericVideoFilter, StripeMaskBase {
public:
	StripeMask(PClip _child, int _blksize, int _blksizev, int _overlap, int _overlapv, int _thr, int comp, int compv, int _str, int _strf, bool _lines, IScriptEnvironment* env);
	~StripeMask() {}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	int __stdcall SetCacheHints(int cachehints, int frame_range);
};

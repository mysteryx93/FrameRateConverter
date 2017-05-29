#include "avisynth.h"
#include "avs\minmax.h"
#include <cstring>

class ContinuousMask : public GenericVideoFilter {
public:
	ContinuousMask(PClip _child, int _radius, IScriptEnvironment* env);
	~ContinuousMask();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	int __stdcall SetCacheHints(int cachehints, int frame_range);
private:
	template<typename T, typename P> void Calculate(const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch, IScriptEnvironment* env);

	const int radius;
};

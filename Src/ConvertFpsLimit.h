#include "avisynth.h"
#include <stdint.h>
#include <cmath>
#include "avs/win.h"
#include "avs/minmax.h"
#include "merge.h"


class ConvertFPS : public GenericVideoFilter
	/**
	  * Class to change the framerate, attempting to smooth the transitions
	 **/
{
public:
	ConvertFPS(PClip _child, unsigned new_numerator, unsigned new_denominator, int _zone,
		int _vbi, int _ratio, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

	int __stdcall SetCacheHints(int cachehints, int frame_range) override {
		return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
	}

	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreateFloat(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreatePreset(AVSValue args, void*, IScriptEnvironment* env);
	static AVSValue __cdecl CreateFromClip(AVSValue args, void*, IScriptEnvironment* env);

private:
	int64_t fa, fb;
	int zone;
	//Variables used in switch mode only
	int vbi;    //Vertical Blanking Interval (lines)
	int lps;    //Lines per source frame
	int ratio;
};

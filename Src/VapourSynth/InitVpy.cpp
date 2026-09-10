#include "VapourSynth.h"
#include "VSHelper.h"
#include "StripeMaskVpy.h"
#include "ContinuousMaskVpy.h"
#include "ConvertFpsLimitVpy.h"
#include "../Common/FrcVersion.h"

static void VS_CC FrcVersionCreate(const VSMap *, VSMap *out, void *, VSCore *, const VSAPI *vsapi)
{
	vsapi->propSetData(out, "version", FRC_VERSION_STRING, -1, paReplace);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
	configFunc("com.vapoursynth.frc", "frc", "Frame Rate Converter " FRC_VERSION_STRING, VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("Version", "", FrcVersionCreate, 0, plugin);
	registerFunc("ContinuousMask",
		"clip:clip;"
		"radius:int:opt;"
		"thr:int:opt;",
		ContinuousMaskVpy::Create, 0, plugin);
	registerFunc("StripeMaskPass", 
		"clip:clip;"
		"blksize:int:opt;"
		"blksizev:int:opt;"
		"overlap:int:opt;"
		"overlapv:int:opt;"
		"thr:int:opt;"
		"range:int:opt;"
		"gamma:float:opt;"
		"comp:int:opt;"
		"compv:int:opt;"
		"str:int:opt;"
		"lines:int:opt;",
		StripeMaskVpy::Create, 0, plugin);
	registerFunc("ConvertFpsLimit",
		"clip:clip;"
		"num:int:opt;"
		"den:int:opt;"
		"fps:float:opt;"
		"preset:data:opt;"
		"match:clip:opt;"
		"ratio:int:opt;",
		ConvertFpsLimitVpy::Create, 0, plugin);
}

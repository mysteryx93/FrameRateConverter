#include "VapourSynth.h"
#include "VSHelper.h"
#include "ContinuousMaskVpy.h"
#include "StripeMaskVpy.h"
#include "ConvertFpsLimitVpy.h"

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
	configFunc("com.vapoursynth.frc", "frc", "Frame Rate Connverter", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("ContinuousMask",
		"clip:clip;"
		"radius:int:opt;",
		ContinuousMaskVpy::Create, 0, plugin);
	registerFunc("StripeMask", 
		"clip:clip;"
		"blkSize:int:opt;"
		"blkSizeV:int:opt;"
		"overlap:int:opt;"
		"overlapV:int:opt;"
		"thr:int:opt;"
		"comp:int:opt;"
		"compV:int:opt;"
		"str:int:opt;"
		"strf:int:opt;"
		"fullRange:int:opt;"
		"lines:int:opt;",
		StripeMaskVpy::Create, 0, plugin);
	registerFunc("ConvertFpsLimit",
		"clip:clip;"
		"num:int;"
		"den:int;"
		"ratio:int:opt;",
		ConvertFpsLimitVpy::Create, 0, plugin);
	registerFunc("ConvertFpsLimit",
		"clip:clip;"
		"fps:float;"
		"ratio:int:opt;",
		ConvertFpsLimitVpy::CreateFloat, 0, plugin);
	registerFunc("ConvertFpsLimit",
		"clip:clip;"
		"fps:data;"
		"ratio:int:opt;",
		ConvertFpsLimitVpy::CreatePreset, 0, plugin);
	registerFunc("ConvertFpsLimit",
		"clip:clip;"
		"fps:clip;"
		"ratio:int:opt;",
		ConvertFpsLimitVpy::CreateFromClip, 0, plugin);
}

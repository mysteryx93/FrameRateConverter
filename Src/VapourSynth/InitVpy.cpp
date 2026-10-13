#include <VapourSynth4.h>
#include "StripeMaskVpy.h"
#include "ContinuousMaskVpy.h"
#include "ConvertFpsLimitVpy.h"
#include "../Common/FrcVersion.h"

static void VS_CC FrcVersionCreate(const VSMap *, VSMap *out, void *, VSCore *, const VSAPI *vsapi)
{
	vsapi->mapSetData(out, "version", FRC_VERSION_STRING, -1, dtUtf8, maReplace);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi)
{
	vspapi->configPlugin("com.vapoursynth.frc", "frc", "Frame Rate Converter " FRC_VERSION_STRING,
		VS_MAKE_VERSION(2, 1), VAPOURSYNTH_API_VERSION, 0, plugin);
	vspapi->registerFunction("Version", "", "version:data;", FrcVersionCreate, nullptr, plugin);
	vspapi->registerFunction("ContinuousMask",
		"clip:vnode;"
		"radius:int:opt;"
		"thr:int:opt;",
		"clip:vnode;", ContinuousMaskVpy::Create, nullptr, plugin);
	vspapi->registerFunction("StripeMaskPass",
		"clip:vnode;"
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
		"fullrange:int:opt;"
		"lines:int:opt;",
		"clip:vnode;", StripeMaskVpy::Create, nullptr, plugin);
	vspapi->registerFunction("ConvertFpsLimit",
		"clip:vnode;"
		"num:int:opt;"
		"den:int:opt;"
		"fps:float:opt;"
		"preset:data:opt;"
		"match:vnode:opt;"
		"ratio:int:opt;",
		"clip:vnode;", ConvertFpsLimitVpy::Create, nullptr, plugin);
}

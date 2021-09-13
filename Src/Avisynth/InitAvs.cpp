#include "avisynth.h"
#include "conditional.h"
#include "StripeMaskAvs.h"
#include "ContinuousMaskAvs.h"
#include "ConvertFpsLimitAvs.h"

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
	AVS_linkage = vectors;
	env->AddFunction("ConditionalFilterMT", "cccsss[show]b", ConditionalFilter::Create, 0);
	env->AddFunction("StripeMask", "c[blkSize]i[blkSizeV]i[overlap]i[overlapV]i[thr]i[range]i[gamma]f[comp]i[compV]i[str]i[strf]i[fullRange]b[lines]b", StripeMaskAvs::Create, 0);
	env->AddFunction("ContinuousMask", "c[radius]i[thr]i", ContinuousMaskAvs::Create, 0);
	env->AddFunction("ConvertFpsLimit", "ci[]i[ratio]i", ConvertFpsLimitAvs::Create, 0);
	env->AddFunction("ConvertFpsLimit", "cf[ratio]i", ConvertFpsLimitAvs::CreateFloat, 0);
	env->AddFunction("ConvertFpsLimit", "cs[ratio]i", ConvertFpsLimitAvs::CreatePreset, 0);
	env->AddFunction("ConvertFpsLimit", "cc[ratio]i", ConvertFpsLimitAvs::CreateFromClip, 0);
	return "FrameRateConverter";
}

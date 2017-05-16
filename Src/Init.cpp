#include "avisynth.h"
#include "conditional.h"
#include "StripeMask.h"

AVSValue __cdecl Create_ConditionalFilterMT(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new ConditionalFilter(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), args[3], args[4], args[5], args[6].AsBool(false), env);
}

AVSValue __cdecl Create_StripeMask(AVSValue args, void* user_data, IScriptEnvironment* env) {
	PClip input = args[0].AsClip();
	int BlkSize = args[1].AsInt(16);
	int BlkSizeV = args[2].AsInt(BlkSize > 0 ? BlkSize : 16);
	int Overlap = args[3].AsInt(BlkSize / 4);
	int OverlapV = args[4].AsInt(args[2].AsInt(BlkSize) / 4);
	int Trh = args[5].AsInt(22);
	int Comp = args[6].AsInt(BlkSize < 16 ? 2 : 3);
	int CompV = args[7].AsInt(args[6].AsInt(BlkSizeV < 16 ? 2 : 3));
	int Str = args[8].AsInt(255);
	int Strf = args[9].AsInt(0);
	bool Lines = args[10].AsBool(false);
	return new StripeMask(input, BlkSize, BlkSizeV, Overlap, OverlapV, Trh, Comp, CompV, Str, Strf, Lines, env);
}

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
	AVS_linkage = vectors;
	env->AddFunction("ConditionalFilterMT", "cccsss[show]b", Create_ConditionalFilterMT, 0);
	env->AddFunction("StripeMask", "c[blksize]i[blksizev]i[overlap]i[overlapv]i[trh]i[Comp]i[CompV]i[str]i[strf]i[lines]b", Create_StripeMask, 0);
	return "ConditionalFilterMT";
}

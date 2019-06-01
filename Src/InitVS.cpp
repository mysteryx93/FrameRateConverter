#include "VapourSynth.h"
#include "VSHelper.h"
#include "conditional.h"
#include "StripeMask.h"
#include "ContinuousMask.h"

typedef struct {
	VSNodeRef *node, *source1, *source2;
	const VSVideoInfo *vi;
	char *condition1, *evaluator, *condition2;
	bool show;
} InvertData;


//
//AVSValue __cdecl Create_ConditionalFilterMT(AVSValue args, void* user_data, IScriptEnvironment* env) {
//	return new ConditionalFilter(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), args[3], args[4], args[5], args[6].AsBool(false), env);
//}
//
//AVSValue __cdecl Create_StripeMask(AVSValue args, void* user_data, IScriptEnvironment* env) {
//	PClip input = args[0].AsClip();
//	int BlkSize = args[1].AsInt(16);
//	int BlkSizeV = args[2].AsInt(BlkSize > 0 ? BlkSize : 16);
//	int Overlap = args[3].AsInt(BlkSize / 4);
//	int OverlapV = args[4].AsInt(args[2].AsInt(BlkSize) / 4);
//	int Thr = args[5].AsInt(26);
//	int Comp = args[6].AsInt(BlkSize <= 16 ? 2 : 3);
//	int CompV = args[7].AsInt(args[6].AsInt(BlkSizeV <= 16 ? 2 : 3));
//	int Str = args[8].AsInt(255);
//	int Strf = args[9].AsInt(0);
//	bool Lines = args[10].AsBool(false);
//
//	// Convert input to linear (gamma 2.2)
//	AVSValue sargs[2] = { input, ((1.0 / 2.2) - 1.0) * 256.0 };
//	const char *nargs[2] = { 0, "gamma_y" };
//	input = env->Invoke("ColorYUV", AVSValue(sargs, 2), nargs).AsClip();
//
//	// Convert input to 8-bit; nothing to gain in processing at higher bit-depth.
//	int SrcBit = input->GetVideoInfo().BitsPerComponent();
//	if (SrcBit > 8) {
//		AVSValue sargs[2] = { input, 8 };
//		const char *nargs[2] = { 0, 0 };
//		input = env->Invoke("ConvertBits", AVSValue(sargs, 2), nargs).AsClip();
//	}
//
//	input = new StripeMask(input, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, Strf, Lines, env);
//
//	// Convert back to original bit depth.
//	if (SrcBit > 8) {
//		AVSValue sargs[2] = { input, SrcBit };
//		const char *nargs[2] = { 0, 0 };
//		input = env->Invoke("ConvertBits", AVSValue(sargs, 2), nargs).AsClip();
//	}
//	return input;
//}
//
//AVSValue __cdecl Create_ContinuousMask(AVSValue args, void* user_data, IScriptEnvironment* env) {
//	return new ContinuousMask(args[0].AsClip(), args[1].AsInt(16), env);
//}
//
//const AVS_Linkage *AVS_linkage = 0;
//
//extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
//	AVS_linkage = vectors;
//	env->AddFunction("ConditionalFilterMT", "cccsss[show]b", Create_ConditionalFilterMT, 0);
//	env->AddFunction("StripeMask", "c[blksize]i[blksizev]i[overlap]i[overlapv]i[thr]i[Comp]i[CompV]i[str]i[strf]i[lines]b", Create_StripeMask, 0);
//	env->AddFunction("ContinuousMask", "c[radius]i", Create_ContinuousMask, 0);
//	return "ConditionalFilterMT";
//}

// This function is responsible for validating arguments and creating a new filter
static void VS_CC invertCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	InvertData d;
	InvertData *data;
	int err;

	// Get a clip reference from the input arguments. This must be freed later.
	d.node = vsapi->propGetNode(in, "clip", 0, 0);
	d.vi = vsapi->getVideoInfo(d.node);

	// In this first version we only want to handle 8bit integer formats. Note that
	// vi->format can be 0 if the input clip can change format midstream.
	if (!isConstantFormat(d.vi) || d.vi->format->sampleType != stInteger || d.vi->format->bitsPerSample != 8) {
		vsapi->setError(out, "Invert: only constant format 8bit integer input supported");
		vsapi->freeNode(d.node);
		return;
	}

	// If a property read fails for some reason (index out of bounds/wrong type)
	// then err will have flags set to indicate why and 0 will be returned. This
	// can be very useful to know when having optional arguments. Since we have
	// strict checking because of what we wrote in the argument string, the only
	// reason this could fail is when the value wasn't set by the user.
	// And when it's not set we want it to default to enabled.
	d.enabled = !!vsapi->propGetInt(in, "enable", 0, &err);
	if (err)
		d.enabled = 1;

	// Let's pretend the only allowed values are 1 or 0...
	if (d.enabled < 0 || d.enabled > 1) {
		vsapi->setError(out, "Invert: enabled must be 0 or 1");
		vsapi->freeNode(d.node);
		return;
	}

	// I usually keep the filter data struct on the stack and don't allocate it
	// until all the input validation is done.
	data = malloc(sizeof(d));
	*data = d;

	// Creates a new filter and returns a reference to it. Always pass on the in and out
	// arguments or unexpected things may happen. The name should be something that's
	// easy to connect to the filter, like its function name.
	// The three function pointers handle initialization, frame processing and filter destruction.
	// The filtermode is very important to get right as it controls how threading of the filter
	// is handled. In general you should only use fmParallel whenever possible. This is if you
	// need to modify no shared data at all when the filter is running.
	// For more complicated filters, fmParallelRequests is usually easier to achieve as it can
	// be prefetched in parallel but the actual processing is serialized.
	// The others can be considered special cases where fmSerial is useful to source filters and
	// fmUnordered is useful when a filter's state may change even when deciding which frames to
	// prefetch (such as a cache filter).
	// If your filter is really fast (such as a filter that only resorts frames) you should set the
	// nfNoCache flag to make the caching work smoother.
	vsapi->createFilter(in, out, "Invert", invertInit, invertGetFrame, invertFree, fmParallel, 0, data, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
	configFunc("com.Etienne.FrameRateConverter", "frc", "Frame Rate Connverter", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("ConditionalFilterMT", 
		"clip:clip;"
		"source1:clip;"
		"source2:clip;"
		"condition1:data;"
		"evaluator:data;"
		"condition2:data;"
		"show:int:opt",
		invertCreate, 0, plugin);
}

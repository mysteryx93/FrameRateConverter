#include "StripeMaskAvs.h"

AVSValue __cdecl StripeMaskAvs::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	PClip input = args[0].AsClip();
	int BlkSize = args[1].AsInt(16);
	int BlkSizeV = args[2].AsInt(BlkSize > 0 ? BlkSize : 16);
	int Overlap = args[3].AsInt(BlkSize / 4);
	int OverlapV = args[4].AsInt(BlkSizeV / 4);
	int Thr = args[5].AsInt(28);
	int Comp = args[6].AsInt(BlkSize <= 16 ? 2 : 3);
	int CompV = args[7].AsInt(Comp);
	int Str = args[8].AsInt(255);
	int Strf = args[9].AsInt(0);
	int FullRange = args[10].AsBool(false);
	bool Lines = args[11].AsBool(false);

	int SrcBit = input->GetVideoInfo().BitsPerComponent();

	// Convert to 16-bit.
	{
		AVSValue sargs[2] = { input, 16 };
		const char* nargs[2] = { 0, 0 };
		input = env->Invoke("ConvertBits", AVSValue(sargs, 2), nargs).AsClip();
	}

	// Convert to Y
	{
		AVSValue sargs[1] = { input };
		const char* nargs[1] = { 0 };
		input = env->Invoke("ConvertToY", AVSValue(sargs, 1), nargs).AsClip();
	}

	// Convert to Full levels.
	if (!FullRange)
	{
		AVSValue sargs[2] = { input, "TV->PC" };
		const char* nargs[2] = { 0, "levels" };
		input = env->Invoke("ColorYUV", AVSValue(sargs, 2), nargs).AsClip();
	}

	// Convert input to linear (gamma 2.2)
	{
		AVSValue sargs[6] = { input, 0, 1.0/2.2, 65535, 0, 65535 };
		const char* nargs[6] = { 0, 0, 0, 0, 0, 0 };
		input = env->Invoke("Levels", AVSValue(sargs, 6), nargs).AsClip();
	}

	// Convert input to 8-bit; nothing to gain in processing at higher bit-depth.
	{
		AVSValue sargs[2] = { input, 8 };
		const char* nargs[2] = { 0, 0 };
		input = env->Invoke("ConvertBits", AVSValue(sargs, 2), nargs).AsClip();
	}

	input = new StripeMaskAvs(input, BlkSize, BlkSizeV, Overlap, OverlapV, Thr, Comp, CompV, Str, Strf, Lines, env);

	// Convert back to original bit depth.
	if (SrcBit > 8)
	{
		AVSValue sargs[2] = { input, SrcBit };
		const char* nargs[2] = { 0, 0 };
		input = env->Invoke("ConvertBits", AVSValue(sargs, 2), nargs).AsClip();
	}
	return input;
}

StripeMaskAvs::StripeMaskAvs(PClip _child, int _blksize, int _blksizev, int _overlap, int _overlapv, int _thr, int _comp, int _compv, int _str, int _strf, bool _lines, IScriptEnvironment* env) :
	GenericVideoFilter(_child), StripeMaskBase(new AvsVideo(_child), AvsEnvironment(PluginName, env), _blksize, _blksizev, _overlap, _overlapv, _thr, _comp, _compv, _str, _strf, _lines)
{
	int b = vi.BitsPerComponent();
	vi.pixel_type = b == 8 ? VideoInfo::CS_Y8 : b == 10 ? VideoInfo::CS_Y10 : b == 12 ? VideoInfo::CS_Y12 : b == 14 ? VideoInfo::CS_Y14 : b == 16 ? VideoInfo::CS_Y16 : b == 32 ? VideoInfo::CS_Y32 : VideoInfo::CS_Y8;
}

PVideoFrame __stdcall StripeMaskAvs::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame src2 = nullptr;
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (strf > 0 && n < vi.num_frames - 1)
	{
		src2 = child->GetFrame(n + 1, env);
	}
	ProcessFrame(AvsFrame(src, vi), AvsFrame(src2, vi), AvsFrame(dst, vi), AvsEnvironment(PluginName, env));
	return dst;
}

// Marks filter as multi-threading friendly.
int __stdcall StripeMaskAvs::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

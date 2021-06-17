#include "ConvertFpsLimitAvs.h"

ConvertFpsLimitAvs::ConvertFpsLimitAvs(PClip _child, unsigned new_numerator, unsigned new_denominator, int _ratio, IScriptEnvironment* env) :
	GenericVideoFilter(_child),
	ConvertFPSLimitBase(new AvsVideo(_child), AvsEnvironment(env), new_numerator, new_denominator, _ratio)
{
	vi.SetFPS(new_numerator, new_denominator);
	const int64_t num_frames = (vi.num_frames * fb + (fa >> 1)) / fa;
	if (num_frames > 0x7FFFFFFF)  // MAXINT
		env->ThrowError("ConvertFpsLimit: Maximum number of frames exceeded.");
	vi.num_frames = int(num_frames);
}

PVideoFrame __stdcall ConvertFpsLimitAvs::GetFrame(int n, IScriptEnvironment* env)
{
	int nsrc = int(n * fa / fb);
	PVideoFrame src = child->GetFrame(nsrc, env);
	PVideoFrame src2 = child->GetFrame(nsrc + 1, env);
	env->MakeWritable(&src);
	ICommonFrame& Result = ProcessFrame(n, AvsFrame(src, vi), AvsFrame(src2, vi), AvsEnvironment(env));
	return *(PVideoFrame*)Result.Ref;
}

bool __stdcall ConvertFpsLimitAvs::GetParity(int n)
{
	if (vi.IsFieldBased())
		return child->GetParity(0) ^ (n & 1);
	else
		return child->GetParity(0);
}

AVSValue __cdecl ConvertFpsLimitAvs::Create(AVSValue args, void*, IScriptEnvironment* env)
{
	return new ConvertFpsLimitAvs(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(1), args[3].AsInt(100), env);
}

AVSValue __cdecl ConvertFpsLimitAvs::CreateFloat(AVSValue args, void*, IScriptEnvironment* env)
{
	uint32_t num, den;
	ConvertFpsLimitAvs::FloatToFPS("ConvertFpsLimit", (float)args[1].AsFloat(), num, den, AvsEnvironment(env));
	return new ConvertFpsLimitAvs(args[0].AsClip(), num, den, args[2].AsInt(100), env);
}

// Tritical Jan 2006
AVSValue __cdecl ConvertFpsLimitAvs::CreatePreset(AVSValue args, void*, IScriptEnvironment* env)
{
	uint32_t num, den;
	ConvertFpsLimitAvs::PresetToFPS("ConvertFpsLimit", args[1].AsString(), num, den, AvsEnvironment(env));
	return new ConvertFpsLimitAvs(args[0].AsClip(), num, den, args[2].AsInt(100), env);
}

AVSValue __cdecl ConvertFpsLimitAvs::CreateFromClip(AVSValue args, void*, IScriptEnvironment* env)
{
	const VideoInfo& vi = args[1].AsClip()->GetVideoInfo();

	if (!vi.HasVideo())
	{
		env->ThrowError("ConvertFpsLimit: The clip supplied to get the FPS from must contain video.");
	}

	return new ConvertFpsLimitAvs(args[0].AsClip(), vi.fps_numerator, vi.fps_denominator, args[2].AsInt(100), env);
}

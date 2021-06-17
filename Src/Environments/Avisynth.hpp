#pragma once
#include "../Avisynth/avisynth.h"
#include "Common.h"
#include <string>

struct AvsEnvironment : public ICommonEnvironment {
	IScriptEnvironment* Env;

	AvsEnvironment(IScriptEnvironment* _env) :
		Env(_env)
	{
	}

	void ThrowError(const char* message)
	{
		Env->ThrowError(message);
	}

	void MakeWritable(ICommonFrame& frame)
	{
		Env->MakeWritable((PVideoFrame*)frame.Ref);
	}
};

struct AvsVideo : public ICommonVideo
{
	const VideoInfo VInfo;

	AvsVideo(PClip _video) :
		ICommonVideo(_video), 
		VInfo(_video ? _video->GetVideoInfo() : VideoInfo())
	{
	}

	int FpsNum()
	{
		return VInfo.fps_numerator;
	}

	int FpsDen()
	{
		return VInfo.fps_denominator;
	}

	int Height()
	{
		return VInfo.height;
	}

	int Width()
	{
		return VInfo.width;
	}

	int NumFrames()
	{
		return VInfo.num_frames;
	}

	int NumPlanes()
	{
		return IsPlanar() ? VInfo.NumComponents() : 1;
	}

	int BitsPerSample()
	{
		return VInfo.BitsPerComponent();
	}

	bool IsPlanar()
	{
		return VInfo.IsPlanar();
	}

	bool IsY()
	{
		return VInfo.IsY();
	}

	bool IsYUV()
	{
		return VInfo.IsYUV();
	}

	bool IsRGB()
	{
		return VInfo.IsRGB();
	}

	bool IsYUY2()
	{
		return VInfo.IsYUY2();
	}

	bool HasAlpha()
	{
		return VInfo.NumComponents() == 4;
	}
};

struct AvsFrame : public ICommonFrame
{
	const PVideoFrame& Frame;
	const VideoInfo VInfo;

	AvsFrame(PVideoFrame& _frame, const VideoInfo _vi) :
		ICommonFrame(&_frame),
		Frame(_frame), VInfo(_vi)
	{
	}

	bool HasValue()
	{
		return Frame;
	}

	// Convert plane index to Avisynth plane flag.
	int GetPlane(int plane)
	{
		if (VInfo.IsPlanar())
		{
			if (VInfo.IsYUV())
			{
				return plane == 0 ? PLANAR_Y : plane == 1 ? PLANAR_U : plane == 2 ? PLANAR_V : plane == 3 ? PLANAR_A : throw std::string("Invalid plane index.");
			}
			else if (VInfo.IsRGB())
			{
				return plane == 0 ? PLANAR_R : plane == 1 ? PLANAR_G : plane == 2 ? PLANAR_B : plane == 3 ? PLANAR_A : throw std::string("Invalid plane index.");
			}
		}
		return plane == 0 ? 0 : throw std::string("Invalid plane index.");
	}

	int GetStride(int plane = 0)
	{
		return Frame->GetPitch(GetPlane(plane));
	}

	int GetRowSize(int plane = 0)
	{
		return Frame->GetRowSize(GetPlane(plane));
	}

	int GetWidth(int plane = 0)
	{
		return Frame->GetRowSize(GetPlane(plane)) / BytesPerSample();
	}

	int GetHeight(int plane = 0)
	{
		return Frame->GetHeight(GetPlane(plane));
	}

	int BitsPerSample()
	{
		return VInfo.BitsPerComponent();
	}

	int BytesPerSample()
	{
		return VInfo.BitsPerComponent();
	}

	BYTE* GetWritePtr(int plane = 0)
	{
		return Frame->GetWritePtr(GetPlane(plane));
	}

	const BYTE* GetReadPtr(int plane = 0)
	{
		return Frame->GetReadPtr(GetPlane(plane));
	}
};
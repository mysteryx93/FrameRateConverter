#pragma once
#include "../VapourSynth/VapourSynth.h"
#include "Common.h"
#include "../VapourSynth/VSHelper.h"
#include <string>

struct VpyEnvironment : public ICommonEnvironment {
	const VSAPI* Api;
	VSCore* Core;
	VSMap* Out;

	VpyEnvironment(const VSAPI* _api, VSCore* _core, VSMap* _out) :
		Api(_api), Out(_out), Core(_core)
	{
	}

	void ThrowError(const char* message)
	{
		throw std::string(message);
		// Api->setError(Out, message);
	}

	void MakeWritable(ICommonFrame& frame)
	{
		frame.Ref = Api->copyFrame(static_cast<VSFrameRef*>(frame.Ref), Core);
	}
};

struct VpyVideo : ICommonVideo
{
	void* Ref;
	const VSVideoInfo* VInfo;

	VpyVideo(VSNodeRef* _video, const VSAPI* _api) :
		Ref(_video), VInfo(_video ? _api->getVideoInfo(_video) : nullptr)
	{
	}

	int FpsNum()
	{
		return VInfo->fpsNum;
	}

	int FpsDen()
	{
		return VInfo->fpsDen;
	}

	int Height()
	{
		return VInfo->height;
	}

	int Width()
	{
		return VInfo->width;
	}

	int NumFrames()
	{
		return VInfo->numFrames;
	}

	int NumPlanes()
	{
		return VInfo->format->numPlanes;
	}

	int BitsPerSample()
	{
		return VInfo->format->bitsPerSample;
	}

	bool IsPlanar()
	{
		return VInfo->format->numPlanes > 0;
	}

	bool IsY()
	{
		return VInfo->format->colorFamily == cmGray;
	}

	bool IsYUV()
	{
		return VInfo->format->colorFamily == cmYUV;
	}

	bool IsRGB()
	{
		return VInfo->format->colorFamily == cmRGB;
	}

	bool IsYUY2()
	{
		return VInfo->format->id == pfCompatYUY2;
	}

	bool HasAlpha()
	{
		return VInfo->format->numPlanes == 4 || VInfo->format->id == pfCompatBGR32;
	}
};

struct VpyFrame : public ICommonFrame
{
	//void* Ref;
	const VSAPI* Api;
	const VSFrameRef* Frame;

	VpyFrame(VSFrameRef* _frame, const VSAPI* _api) :
		ICommonFrame(_frame),
		Api(_api), Frame(_frame)
	{
	}

	VpyFrame(const VSFrameRef* _frame, const VSAPI* _api) :
		ICommonFrame((VSFrameRef*)_frame),
		Api(_api), Frame(_frame)
	{
	}

	bool HasValue()
	{
		return Frame;
	}

	int GetStride(int plane = 0)
	{
		return Api->getStride(Frame, plane);
	}

	int GetRowSize(int plane = 0)
	{
		return GetWidth(plane) * BytesPerSample();
	}

	int GetWidth(int plane = 0)
	{
		return Api->getFrameWidth(Frame, plane);
	}

	int GetHeight(int plane = 0)
	{
		return Api->getFrameHeight(Frame, plane);
	}

	int BitsPerSample()
	{
		return Api->getFrameFormat(Frame)->bitsPerSample;
	}

	int BytesPerSample()
	{
		return Api->getFrameFormat(Frame)->bytesPerSample;
	}

	BYTE* GetWritePtr(int plane = 0)
	{
		return Api->getWritePtr((VSFrameRef*)Frame, plane);
	}

	const BYTE* GetReadPtr(int plane = 0)
	{
		return Api->getReadPtr(Frame, plane);
	}
};

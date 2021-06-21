#pragma once
#include "../VapourSynth/VapourSynth.h"
#include "Common.h"
#include "../VapourSynth/VSHelper.h"
#include <string>
#include <stdexcept>

struct VpyEnvironment : public ICommonEnvironment
{
	const VSAPI* Api;
	VSCore* Core;
	VSMap* Out = nullptr;
	VSFrameContext* FrameCtx = nullptr;

	VpyEnvironment(const char* pluginName, const VSAPI* _api, VSCore* _core, VSMap* out) :
		ICommonEnvironment(pluginName),
		Api(_api), Core(_core), Out(out)
	{
	}

	VpyEnvironment(const char* pluginName, const VSAPI* _api, VSCore* _core, VSFrameContext* frameCtx) :
		ICommonEnvironment(pluginName),
		Api(_api), Core(_core), FrameCtx(frameCtx)
	{
	}

	void ThrowErrorInternal(const char* message)
	{
		if (Out)
		{
			Api->setError(Out, message);
		}
		else if (FrameCtx)
		{
			Api->setFilterError(message, FrameCtx);
		}
		else
		{
			throw std::runtime_error{ message };
		}
	}

	void MakeWritable(ICommonFrame& frame)
	{
		auto Copy = Api->copyFrame(static_cast<VSFrameRef*>(frame.Ref), Core);
		Api->freeFrame((VSFrameRef*)frame.Ref);
		frame.Ref = Copy;
	}

VSNodeRef* InvokeClip(const char* ns, const char* func, VSMap* args, VSNodeRef* input)
{
	VSNodeRef* Result = nullptr;
	auto Plugin = Api->getPluginByNs(ns, Core);
	auto ret = Api->invoke(Plugin, func, args);
	Api->freeNode(input);
	if (Api->getError(ret))
	{
		ThrowError(Api->getError(ret));
	}
	else
	{
		Result = Api->propGetNode(ret, "clip", 0, NULL);
	}
	Api->freeMap(ret);
	return Result;
}
};

struct VpyVideo : ICommonVideo
{
	const VSVideoInfo* VInfo;

	VpyVideo(VSNodeRef* _video, const VSAPI* _api) :
		ICommonVideo(_video),
		VInfo(_video ? _api->getVideoInfo(_video) : nullptr)
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
	const VSAPI* Api;
	VSFrameRef* Frame()
	{
		return (VSFrameRef*)Ref;
	}

	VpyFrame(VSFrameRef* _frame, const VSAPI* _api) :
		ICommonFrame(_frame),
		Api(_api)
	{
	}

	VpyFrame(const VSFrameRef* _frame, const VSAPI* _api) :
		ICommonFrame((VSFrameRef*)_frame),
		Api(_api)
	{
	}

	bool HasValue()
	{
		return Ref;
	}

	int GetStride(int plane = 0)
	{
		return Api->getStride(Frame(), plane);
	}

	int GetRowSize(int plane = 0)
	{
		return GetWidth(plane) * BytesPerSample();
	}

	int GetWidth(int plane = 0)
	{
		return Api->getFrameWidth(Frame(), plane);
	}

	int GetHeight(int plane = 0)
	{
		return Api->getFrameHeight(Frame(), plane);
	}

	int BitsPerSample()
	{
		return Api->getFrameFormat(Frame())->bitsPerSample;
	}

	int BytesPerSample()
	{
		return Api->getFrameFormat(Frame())->bytesPerSample;
	}

	BYTE* GetWritePtr(int plane = 0)
	{
		return Api->getWritePtr(Frame(), plane);
	}

	const BYTE* GetReadPtr(int plane = 0)
	{
		return Api->getReadPtr(Frame(), plane);
	}
};

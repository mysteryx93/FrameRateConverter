#pragma once
#include <VapourSynth4.h>
#include "Common.h"
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

	void ThrowErrorInternal(const char* message) const
	{
		if (Out)
		{
			Api->mapSetError(Out, message);
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
		auto Copy = Api->copyFrame((VSFrame*)frame.Ref, Core);
		Api->freeFrame((VSFrame*)frame.Ref);
		frame.Ref = Copy;
	}

VSNode* InvokeClip(const char* ns, const char* func, VSMap* args, VSNode* input)
{
	VSNode* Result = nullptr;
	auto Plugin = Api->getPluginByNamespace(ns, Core);
	if (!Plugin)
	{
		Api->freeNode(input);
		ThrowError("required plugin namespace '%s' is not available", ns);
		return nullptr;
	}
	auto ret = Api->invoke(Plugin, func, args);
	Api->freeNode(input);
	if (Api->mapGetError(ret))
	{
		ThrowError("%s", Api->mapGetError(ret));
	}
	else
	{
		Result = Api->mapGetNode(ret, "clip", 0, nullptr);
	}
	Api->freeMap(ret);
	return Result;
}
};

struct VpyVideo : ICommonVideo
{
	const VSVideoInfo* VInfo;

	VpyVideo(VSNode* _video, const VSAPI* _api) :
		ICommonVideo(_video),
		VInfo(_video ? _api->getVideoInfo(_video) : nullptr)
	{
	}

	int64_t FpsNum()
	{
		return VInfo->fpsNum;
	}

	int64_t FpsDen()
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
		return VInfo->format.numPlanes;
	}

	int BitsPerSample()
	{
		return VInfo->format.bitsPerSample;
	}

	bool IsPlanar()
	{
		return VInfo->format.numPlanes > 0;
	}

	bool IsY()
	{
		return VInfo->format.colorFamily == cfGray;
	}

	bool IsYUV()
	{
		return VInfo->format.colorFamily == cfYUV;
	}

	bool IsRGB()
	{
		return VInfo->format.colorFamily == cfRGB;
	}

	bool IsYUY2()
	{
		return false;
	}

	bool HasAlpha()
	{
		return VInfo->format.numPlanes == 4;
	}
};

struct VpyFrame : public ICommonFrame
{
	const VSAPI* Api;
	VSFrame* Frame() const
	{
		return (VSFrame*)Ref;
	}

	VpyFrame(VSFrame* _frame, const VSAPI* _api) :
		ICommonFrame(_frame),
		Api(_api)
	{
	}

	VpyFrame(const VSFrame* _frame, const VSAPI* _api) :
		ICommonFrame(const_cast<VSFrame*>(_frame)),
		Api(_api)
	{
	}

	bool HasValue() const
	{
		return Ref;
	}

	int GetStride(int plane = 0) const
	{
		return Api->getStride(Frame(), plane);
	}

	int GetRowSize(int plane = 0) const
	{
		return GetWidth(plane) * BytesPerSample();
	}

	int GetWidth(int plane = 0) const
	{
		return Api->getFrameWidth(Frame(), plane);
	}

	int GetHeight(int plane = 0) const
	{
		return Api->getFrameHeight(Frame(), plane);
	}

	int BitsPerSample() const
	{
		return Api->getVideoFrameFormat(Frame())->bitsPerSample;
	}

	int BytesPerSample() const
	{
		return Api->getVideoFrameFormat(Frame())->bytesPerSample;
	}

	BYTE* GetWritePtr(int plane = 0) const
	{
		return Api->getWritePtr(Frame(), plane);
	}

	const BYTE* GetReadPtr(int plane = 0) const
	{
		return Api->getReadPtr(Frame(), plane);
	}
};

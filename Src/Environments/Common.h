#pragma once
#include "instrset_detect.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#pragma warning(disable:26812)

typedef unsigned char   BYTE;

struct ICommonVideo
{
	void* Ref = nullptr;
	ICommonVideo(void* ref) :
		Ref(ref)
	{
	}
	virtual ~ICommonVideo() {};
	virtual int FpsNum() = 0;
	virtual int FpsDen() = 0;
	virtual int Height() = 0;
	virtual int Width() = 0;
	virtual int NumFrames() = 0;
	virtual int NumPlanes() = 0;

	virtual int BitsPerSample() = 0;
	virtual bool IsPlanar() = 0;
	virtual bool IsY() = 0;
	virtual bool IsYUV() = 0;
	virtual bool IsRGB() = 0;
	virtual bool IsYUY2() = 0;
	virtual bool HasAlpha() = 0;
};

struct ICommonFrame
{
	void* Ref = nullptr;
	ICommonFrame(void* ref) :
		Ref(ref)
	{
	}
	virtual ~ICommonFrame() {}
	virtual bool HasValue() = 0;
	virtual int GetStride(int plane = 0) = 0;
	virtual int GetRowSize(int plane = 0) = 0;
	virtual int GetWidth(int plane = 0) = 0;
	virtual int GetHeight(int plane = 0) = 0;
	virtual int BitsPerSample() = 0;
	virtual int BytesPerSample() = 0;
	virtual BYTE* GetWritePtr(int plane = 0) = 0;
	virtual const BYTE* GetReadPtr(int plane = 0) = 0;
};

struct ICommonEnvironment
{
	const char* PluginName;

	ICommonEnvironment(const char* pluginName)
	{
		PluginName = pluginName;
	}

	virtual ~ICommonEnvironment() {}

	//void ThrowError(const char* message)
	//{
	//	// Start the error message with plugin name.
	//	const int MaxSize = 512;
	//	const size_t NameLength = strlen(PluginName);
	//	char Buffer[MaxSize]{ 0 };
	//	strcpy(Buffer, PluginName);
	//	Buffer[NameLength] = ':';
	//	Buffer[NameLength + 1] = ' ';

	//	// Add the error message.
	//	strncpy(Buffer + NameLength + 2, message, 512 - NameLength - 2);

	//	ThrowErrorInternal(Buffer);
	//}

	bool ThrowError(const char* format, ...)
	{
		// Start the error message with plugin name.
		const int MaxSize = 512;
		const size_t NameLength = strlen(PluginName);
		char Buffer[MaxSize]{ 0 };
		strcpy_s(Buffer, PluginName);
		Buffer[NameLength] = ':';
		Buffer[NameLength + 1] = ' ';

		// Add the error message.
		va_list args;
		va_start(args, format);
		vsnprintf(Buffer + NameLength + 2, 512 - NameLength - 2, format, args);
		va_end(args);

		ThrowErrorInternal(Buffer);
		return true;
	}

	//virtual void MakeWritable(ICommonFrame& frame) = 0;

	const int GetCpuSupport()
	{
		return instrset_detect();
	}
	//const int ISET_NONE = 0;    // 80386 instruction set
	//const int ISET_SSE = 1;     // SSE (XMM) supported by CPU (not testing for O.S. support)
	//const int ISET_SSE2 = 2;    // SSE2
	//const int ISET_SSE3 = 3;    // SSE3
	//const int ISET_SSSE3 = 4;   // Supplementary SSE3 (SSSE3)
	//const int ISET_SSE41 = 5;   // SSE4.1
	//const int ISET_SSE42 = 6;   // SSE4.2
	//const int ISET_AVX = 7;     // AVX supported by CPU and operating system
	//const int ISET_AVX2 = 8;    // AVX2
	//const int ISET_AVXS12F = 9; // AVX512F
	//const int ISET_AVX512VL = 10; // AVX512VL
	//const int ISET_AVX512BW = 11; // AVX512BW, AVX512DQ

protected:
	virtual void ThrowErrorInternal(const char* message) = 0;
};

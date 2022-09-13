#pragma once
#include "../Environments/VpyFilter.hpp"
#include "../Common/StripeMaskBase.h"

class StripeMaskVpy : public VpyFilter, StripeMaskBase {
public:
	static void VS_CC Create(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* api);
	StripeMaskVpy(const VSMap* in, VSMap* out, VSCore* core, const VSAPI* api, VSNodeRef* node,
		int _blksize, int _blksizev, int _overlap, int _overlapv, int _thr, int _comp, int _compv, int _str, bool _lines);
	~StripeMaskVpy() {}
	virtual void Init(VSMap* in, VSMap* out, VSNode* node, VpyEnvironment& env);
	virtual VSFrameRef* GetFrame(int n, int activationReason, void** frameData, VSFrameContext* frameCtx, VpyEnvironment& env);
	virtual void Free();
};

#pragma once
#include "VapourSynth.hpp"
#include <string>

class VpyPropReader
{
public:
    VpyPropReader(const VSAPI* _api, const VSMap* _map) :
        api(_api), map(_map)
    {
    }

    int64_t GetInt(const char* key, int64_t def = 0)
    {
        int err;
        auto result = api->propGetInt(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    double GetFloat(const char* key, double def = 0)
    {
        int err;
        auto result = api->propGetFloat(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    VSNodeRef* GetNode(const char* key, VSNodeRef* def = nullptr)
    {
        int err;
        auto result = api->propGetNode(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    const VSFrameRef* GetFrame(const char* key, const VSFrameRef* def = nullptr)
    {
        int err;
        auto result = api->propGetFrame(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    VSFuncRef* GetFunc(const char* key, VSFuncRef* def = nullptr)
    {
        int err;
        auto result = api->propGetFunc(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    const char* GetData(const char* key, const char* def = nullptr)
    {
        int err;
        auto result = api->propGetData(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    template<typename T>
    T GetResult(T result, T def, int err)
    {
        return !err ? result : err != peType ? def : throw std::string("Invalid parameter type.");
    }

private:
    const VSAPI* api;
    const VSMap* map;
};

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

    bool Has(const char* key) const
    {
        return api->mapNumElements(map, key) >= 0;
    }

    int64_t GetInt(const char* key, int64_t def = 0)
    {
        int err;
        auto result = api->mapGetInt(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    double GetFloat(const char* key, double def = 0)
    {
        int err;
        auto result = api->mapGetFloat(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    VSNode* GetNode(const char* key, VSNode* def = nullptr)
    {
        int err;
        auto result = api->mapGetNode(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    const VSFrame* GetFrame(const char* key, const VSFrame* def = nullptr)
    {
        int err;
        auto result = api->mapGetFrame(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    VSFunction* GetFunc(const char* key, VSFunction* def = nullptr)
    {
        int err;
        auto result = api->mapGetFunction(map, key, 0, &err);
        return GetResult(result, def, err);
    }

    const char* GetData(const char* key, const char* def = nullptr)
    {
        int err;
        auto result = api->mapGetData(map, key, 0, &err);
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

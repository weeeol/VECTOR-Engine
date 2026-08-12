#pragma once

#include <d3d12.h>
#include <winerror.h>
#include <comdef.h>
#include "Engine/Core/Assert.hpp"
#include <string>

namespace VECTOR {

    inline void SetDebugObjectName(ID3D12Object* pObject, const char* name) {
        if (pObject && name) {
            size_t len = strlen(name) + 1;
            std::wstring wname(len, L'#');
            mbstowcs_s(nullptr, &wname[0], len, name, _TRUNCATE);
            pObject->SetName(wname.c_str());
        }
    }

}

#define DX_CHECK(x) \
    do { \
        HRESULT hr = (x); \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            std::string errMsg = "DirectX 12 Error: " + std::string(err.ErrorMessage()); \
            VECTOR_HALT(errMsg); \
        } \
    } while (0)

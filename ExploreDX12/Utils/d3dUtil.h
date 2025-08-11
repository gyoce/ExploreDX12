#pragma once

// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <windows.h>
#include <wrl.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_4.h>
#include <string>
#include <comdef.h>
#include <dxgidebug.h>

#include "Utils/Logs.h"

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException
{
public:
    DxException() = default;
    DxException(const HRESULT hr, const std::wstring& functionName, const std::wstring& filename, const int lineNumber)
        : ErrorCode(hr), FunctionName(functionName), Filename(filename), LineNumber(lineNumber)
    {
    }

    std::wstring what() const
    {
        const _com_error err(ErrorCode);
        const std::wstring msg = err.ErrorMessage();
        return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
    }

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                   \
{                                                          \
    HRESULT hr__ = (x);                                    \
    std::wstring wfn = AnsiToWString(__FILE__);            \
    if (FAILED(hr__))                                      \
    {                                                      \
        DxException exception{ hr__, L#x, wfn, __LINE__ }; \
        Logs::Error(exception.what());                     \
        throw exception;                                   \
    }                                                      \
}
#endif

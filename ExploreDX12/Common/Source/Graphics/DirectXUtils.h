#pragma once

#include <wrl.h>
#include <d3dx12/d3dx12.h>
#include <d3dcompiler.h>
#include <comdef.h>

#include "Utils/Logs.h"
#include "Graphics/DirectXMathUtils.h"

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

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

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

namespace DirectXUtils
{
    inline UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
}
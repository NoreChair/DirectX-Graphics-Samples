#pragma once
#include <memory>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

typedef struct Texture
{
    UINT m_indexInHeap = 0;
    UINT m_width = 0;
    UINT m_height = 0;
    std::string m_name;
    std::unique_ptr<byte> m_rawData = nullptr;
    ComPtr<ID3D12Resource> m_textureGPU = nullptr;
    ComPtr<ID3D12Resource> m_textureUploader = nullptr;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int incrementSize) {
        auto handle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr = handle.ptr + incrementSize * m_indexInHeap;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int incrementSize) {
        auto handle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr = handle.ptr + incrementSize * m_indexInHeap;
        return handle;
    }

    void DisposeLoader() {
        m_rawData = nullptr;
        m_textureGPU = nullptr;
    }
}Texture;
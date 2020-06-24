#pragma once
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

typedef struct SamplerObject {
    UINT m_indexInHeap = 0;
    ComPtr<ID3D12DescriptorHeap> m_samplerHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int incrementSize) {
        auto handle = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr = handle.ptr + incrementSize * m_indexInHeap;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int incrementSize) {
        auto handle = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr = handle.ptr + incrementSize * m_indexInHeap;
        return handle;
    }
}Sampler;
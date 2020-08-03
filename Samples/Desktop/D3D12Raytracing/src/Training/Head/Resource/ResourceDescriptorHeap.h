#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
class RenderTarget;

struct ResourceDescriptor {
    ResourceDescriptor() : m_cpuHandle(), m_gpuHandle() {}
    ~ResourceDescriptor() = default;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
};

class ResourceDescriptorHeap {
public:
    ResourceDescriptorHeap(ID3D12Device *pDevice, UINT capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    virtual ~ResourceDescriptorHeap();

protected:
    UINT m_descriptorSize;
    UINT m_capacity;
    UINT m_count;
    D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGPU;
    ID3D12Device* m_pDevice;
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
};
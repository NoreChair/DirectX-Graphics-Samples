#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <Resource/ResourceDescriptorHeap.h>

using Microsoft::WRL::ComPtr;

struct RenderTarget : public ResourceDescriptor {
    RenderTarget() : ResourceDescriptor(), m_resource(nullptr) {}

    ComPtr<ID3D12Resource> m_resource;

    UINT GetWidth() {
        return  m_resource->GetDesc().Width;
    }

    UINT GetHeight() {
        return m_resource->GetDesc().Height;
    }

    DXGI_FORMAT GetFormat() {
        return m_resource->GetDesc().Format;
    }
};

class RenderTargetHeap : public ResourceDescriptorHeap {
    UINT m_count;
    std::vector<RenderTarget*> m_pRenderTargets;

public:
    RenderTargetHeap(ID3D12Device* pDevice, UINT capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
        :ResourceDescriptorHeap(pDevice, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, flag), m_count(0), m_pRenderTargets() {
        m_pRenderTargets.resize(capacity);
    }

    virtual ~RenderTargetHeap() {
        for (size_t i = 0; i < m_pRenderTargets.size; i++) {
            if (m_pRenderTargets[i] != nullptr) {
                delete m_pRenderTargets[i];
                m_pRenderTargets[i] = nullptr;
            }
        }
        m_pRenderTargets.clear();
    }

    bool IsFull() {
        return m_count >= m_capacity;
    }

    // create new render target view from  resource
    RenderTarget* CreateRenderTarget(ComPtr<ID3D12Resource> resource) {
        if (IsFull) {
            return false;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heapStartCPU;
        cpuHandle.ptr += m_descriptorSize + m_count;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_heapStartGPU;
        gpuHandle.ptr += m_descriptorSize + m_count;
        ++m_count;

        auto rtv = new RenderTarget();
        rtv->m_resource = resource;
        rtv->m_cpuHandle = cpuHandle;
        rtv->m_gpuHandle = gpuHandle;

        m_pRenderTargets.push_back(rtv);
        return rtv;
    }
};
#include <d3dx12.h>
#include <Resource/ResourceDescriptorHeap.h>


ResourceDescriptorHeap::ResourceDescriptorHeap(ID3D12Device * pDevice, UINT capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    :m_descriptorSize(0), m_capacity(capacity), m_count(0), m_heapStartCPU(), m_heapStartGPU(), m_descriptorHeap(nullptr), m_pDevice(nullptr) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{
        type,
        capacity,
        flag,
        0
    };
    pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_descriptorHeap.GetAddressOf()));
    m_descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_heapStartCPU = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_heapStartGPU = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

ResourceDescriptorHeap::~ResourceDescriptorHeap() {}


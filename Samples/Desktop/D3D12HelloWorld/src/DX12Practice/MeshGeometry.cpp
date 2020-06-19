#include "stdafx.h"
#include "MeshGeometry.h"
#include <exception>
#include <d3dcompiler.h>
#include "d3dx12.h"
MeshGeometry::MeshGeometry(UINT vboByteSize, UINT iboByteSize, ComPtr<ID3D12Device> device)
{
    MeshGeometry(vboByteSize, iboByteSize, 0, device);
}

MeshGeometry::MeshGeometry(UINT vboByteSize, UINT iboByteSize, UINT stride, ComPtr<ID3D12Device> device)
{
    MeshGeometry(vboByteSize, iboByteSize, stride, DXGI_FORMAT_R16_UINT, device);
}

MeshGeometry::MeshGeometry(UINT vboByteSize, UINT iboByteSize, UINT stride, DXGI_FORMAT indexFormat, ComPtr<ID3D12Device> device) :m_vertexBufferByteSize(vboByteSize), m_indexBufferByteSize(iboByteSize), m_vertexStride(stride), m_indexFormat(indexFormat)
{
    D3DCreateBlob(m_vertexBufferByteSize, m_vertexBufferCPU.GetAddressOf());
    D3DCreateBlob(m_indexBufferByteSize, m_indexBufferCPU.GetAddressOf());

    CD3DX12_HEAP_PROPERTIES defalutHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploaderHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

    device->CreateCommittedResource(&defalutHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_vertexBufferGPU.GetAddressOf()));
    device->CreateCommittedResource(&defalutHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_indexBufferGPU.GetAddressOf()));
    device->CreateCommittedResource(&uploaderHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_vertexBufferUploader.GetAddressOf()));
    device->CreateCommittedResource(&uploaderHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_indexBufferUploader.GetAddressOf()));
}

MeshGeometry::~MeshGeometry()
{
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::GetVextexBufferView() const
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {
        m_vertexBufferGPU->GetGPUVirtualAddress(),
        m_vertexBufferByteSize,
        m_vertexStride
    };
    return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::GetIndexBufferView() const
{
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {
        m_indexBufferGPU->GetGPUVirtualAddress(),
        m_indexBufferByteSize,
        m_indexFormat
    };
    return indexBufferView;
}

void MeshGeometry::UpdateVertexBufferCPU(void * data, UINT byteSize, UINT offset)
{
    if (disposed) {
        throw std::logic_error("Uploader has been disposed");
    }
}

void MeshGeometry::UpdateIndexBuferCPU(void * data, UINT byteSize, UINT offset)
{
    if (disposed) {
        throw std::logic_error("Uploader has been disposed");
    }
}

void MeshGeometry::UploadVertexBufferGPU(ID3D12GraphicsCommandList * commandList)
{
    if (disposed) {
        throw std::logic_error("Uploader has been disposed");
    }

}

void MeshGeometry::UploadIndexBufferGPU(ID3D12GraphicsCommandList * commandList)
{
    if (disposed) {
        throw std::logic_error("Uploader has been disposed");
    }
}

void MeshGeometry::DisposeLoader()
{
    if (!disposed) {
        m_vertexBufferCPU = nullptr;
        m_indexBufferCPU = nullptr;
        m_vertexBufferUploader = nullptr;
        m_indexBufferUploader = nullptr;
    }
}

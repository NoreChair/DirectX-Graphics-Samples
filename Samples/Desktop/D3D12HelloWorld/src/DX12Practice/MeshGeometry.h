#pragma once

#include <string>
#include <unordered_map>
#include <d3d12.h>
#include <DirectXCollision.h>
#include <wrl.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

typedef struct SubMeshGeometry
{
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    UINT baseVertexLocation = 0;
    BoundingBox bounds;
}SubMeshGeometry;

class MeshGeometry {
public:
    std::string m_name;

private:
    bool disposed = false;
    UINT m_vertexStride = 0;
    UINT m_vertexBufferByteSize = 0;
    UINT m_indexBufferByteSize = 0;
    DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;

    ComPtr<ID3DBlob> m_vertexBufferCPU = nullptr;
    ComPtr<ID3DBlob> m_indexBufferCPU = nullptr;
    ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
    ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;
    ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
    ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
    ComPtr<ID3D12CommandList> m_commandList = nullptr;
    std::unordered_map<std::string, SubMeshGeometry> m_subMesh;

public:
    explicit MeshGeometry(UINT vboByteSize, UINT iboByteSize, ComPtr<ID3D12Device> device);
    explicit MeshGeometry(UINT vboByteSize, UINT iboByteSize, UINT stride, ComPtr<ID3D12Device> device);
    explicit MeshGeometry(UINT vboByteSize, UINT iboByteSize, UINT stride, DXGI_FORMAT indexFormat, ComPtr<ID3D12Device> device);
    ~MeshGeometry();

    MeshGeometry(const MeshGeometry&) = delete;
    MeshGeometry& operator=(const MeshGeometry& rhs) = delete;

    D3D12_VERTEX_BUFFER_VIEW GetVextexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
    void UpdateVertexBufferCPU(void* data, UINT byteSize, UINT offset);
    void UpdateIndexBuferCPU(void* data, UINT byteSize, UINT offset);
    void UploadVertexBufferGPU(ID3D12GraphicsCommandList* commandList);
    void UploadIndexBufferGPU(ID3D12GraphicsCommandList* commandList);
    void DisposeLoader();
};


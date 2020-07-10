#pragma once
#include "DXSample.h"
#include <DirectXMath.h>

using namespace DirectX;
struct IDxcBlob;

class SimpleTriangle :public DXSample {
    struct AccelerationStructureBuffer
    {
        ComPtr<ID3D12Resource> pScratch;
        ComPtr<ID3D12Resource> pResult;
        ComPtr<ID3D12Resource> pInstanceDesc;
    };

    struct Vertex
    {
        XMFLOAT3 position;
    };

    typedef UINT16 Index;

public:
    SimpleTriangle(UINT width, UINT height, std::wstring name);

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnSizeChanged(UINT width, UINT height, bool minimized) override;
    virtual void OnDestroy() override;

    virtual void OnDeviceLost();
    virtual void OnDeviceRestored();

private:
    void CreateDeviceResource();
    void CreateRayTracingOutput();
    void CreateVertexIndexBuffer();
    void CreateAccelerationStructure();
    void CreateRootSignature();
    void CreateShaderTables();
    void CreateDXRPipeline();

    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC & desc, ID3D12Device* device, ComPtr<ID3D12RootSignature>& rootSig);

    IDxcBlob * CompileShaderLibrary(const std::string & shaderStr, const std::wstring & name);

private:
    static const UINT s_frameCount = 2;
    const std::wstring c_rayGenShaderName = L"raygen";
    const std::wstring c_missShaderName = L"miss";
    const std::wstring c_closestHitShaderName = L"closesthit";
    const std::wstring c_anyHitShaderName = L"anyhit";
    const std::wstring c_hitGroupName = L"hitgroup";


    UINT m_csuSize;
    UINT m_shaderIdentifierSize;

    ComPtr<ID3D12Device5> m_dxrDevice;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList;

    ComPtr<ID3D12StateObject> m_dxrPipeline;
    ComPtr<ID3D12RootSignature> m_rayGenRootSignature;
    ComPtr<ID3D12RootSignature> m_hitGroupRootSignature;
    ComPtr<ID3D12RootSignature> m_missRootSignature;
    ComPtr<ID3D12RootSignature> m_globalDummySignature;

    ComPtr<ID3D12DescriptorHeap> m_uavHeap;

    ComPtr<ID3D12Resource> m_raygenShaderTable;
    ComPtr<ID3D12Resource> m_missShaderTable;
    ComPtr<ID3D12Resource> m_hitgroupShaderTable;
    ComPtr<ID3D12Resource> m_dxrOutput;
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;


    std::unique_ptr<GpuUploadBuffer> m_vertexUploader;
    std::unique_ptr<GpuUploadBuffer> m_indexUploader;
    AccelerationStructureBuffer m_bottomLevelAS;
    AccelerationStructureBuffer m_topLevelAS;
};
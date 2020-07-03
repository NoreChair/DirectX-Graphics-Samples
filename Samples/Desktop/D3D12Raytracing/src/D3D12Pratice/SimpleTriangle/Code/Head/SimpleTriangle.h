#pragma once
#include "DXSample.h"
#include <DirectXMath.h>

using namespace DirectX;

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
    void CreateVertexIndexBuffer();
    void CreateAccelerationStructure();
    void CreateRootSignature();
    void CreateRayTracingOutput();
    void CreateShaderLibrary();
    void CreateDXRPipeline();

private:
    static const UINT s_frameCount = 2;

    ComPtr<ID3D12Device5> m_dxrDevice;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList;

    ComPtr<ID3D12StateObject> m_dxrStateObject;
    ComPtr<ID3D12RootSignature> m_rayGenRootSignature;
    ComPtr<ID3D12RootSignature> m_hitGroupRootSignature;
    ComPtr<ID3D12RootSignature> m_missRootSignature;

    ComPtr<ID3D12DescriptorHeap> m_uavHeap;

    ComPtr<ID3D12Resource> m_dxrOutput;
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;

    AccelerationStructureBuffer m_bottomLevelAS;
    AccelerationStructureBuffer m_topLevelAS;
};
#pragma once

#include "DXSample.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class DX12Practice :public DXSample
{
    typedef struct Vertex
    {
        XMFLOAT3 position;
    }Vertex;

public:
    DX12Practice(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    void CreatePipeline();
    void CreateAsset();

    void CreateDescriptorHeap();
    void CreateRenderTargetView();
    void CreateDepthStencilView();
    void CreateVertexIndexBuffer();

    void PopulateCommandList();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForDSV();

    // Debug Log
    void LogAdapter(IDXGIFactory4* dxgiFactory);
    void LogDisplyOutput(IDXGIAdapter* adapter);
    void LogDisplyModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
    static const UINT s_frameCount = 2;

    UINT m_frameIndex;
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_srvDescriptorSize;

    UINT64 m_fenceValue;
    HANDLE m_fenceEvent;

    // base device
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;

    // command 
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // synchronization
    ComPtr<ID3D12Fence> m_fence;

    // resource && descriptor && pipeline 
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12Resource> m_renderTargets[s_frameCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
};


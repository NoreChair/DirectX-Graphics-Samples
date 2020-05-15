#pragma once

#include "DXSample.h"

using Microsoft::WRL::ComPtr;

class DX12Practice :public DXSample
{
public:
    DX12Practice(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    void CreatePipeline();
    void CreateAsset();
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

    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12Fence> m_fence;
    ComPtr<ID3D12Resource> m_renderTargets[s_frameCount];
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
};


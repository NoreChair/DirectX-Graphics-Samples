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

private:
    static const UINT s_frameCount = 2;

    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<IDXGIAdapter> m_adapter;
    ComPtr<ID3D12Resource> m_renderTargets[s_frameCount];
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
};


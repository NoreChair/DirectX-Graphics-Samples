// DX12Practice.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "DX12Practice.h"

DX12Practice::DX12Practice(UINT width, UINT height, std::wstring name) : DXSample(width, height, name)
{
}

void DX12Practice::OnInit()
{
    CreatePipeline();
}


void DX12Practice::OnUpdate()
{
}

void DX12Practice::OnRender()
{
}

void DX12Practice::OnDestroy()
{
    CloseHandle(m_fenceEvent);
}

void DX12Practice::CreatePipeline()
{
    UINT flags = 0;
#if _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> dxgiFactory;
    ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)));

#if _DEBUG
    LogAdapter(dxgiFactory.Get());
#endif 

    GetHardwareAdapter(dxgiFactory.Get(), &m_adapter);
    ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    {
        D3D12_COMMAND_QUEUE_DESC  desc = { };
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
    }

    {
        //D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
        //msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        //msQualityLevels.SampleCount = 4;
        //msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        //msQualityLevels.NumQualityLevels = 0;
        //ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
        //assert(msQualityLevels.NumQualityLevels > 0);

        ComPtr<IDXGISwapChain1> swapChain;
        DXGI_SWAP_CHAIN_DESC1 desc = { };
        desc.BufferCount = s_frameCount;
        desc.Width = m_width;
        desc.Height = m_height;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc.Count = 1;

        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), Win32Application::GetHwnd(), &desc, nullptr, nullptr, &swapChain));
        ThrowIfFailed(swapChain.As(&m_swapChain));
    }

    ThrowIfFailed(dxgiFactory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;
        m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        if (m_fenceEvent == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    // we don't use command list,so close it now
    m_commandList->Close();
}

void DX12Practice::CreateAsset()
{
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDesc.NumDescriptors = s_frameCount;
    rtvDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDesc.NumDescriptors = 1;
    dsvDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.GetAddressOf())));

    // Create Render Target View
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < s_frameCount; ++i)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf()));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
        handle.Offset(1, m_rtvDescriptorSize);
    }

    // Create Depth/Stencil buffer
    D3D12_RESOURCE_DESC dsvResourceDesc = { };
    dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsvResourceDesc.Alignment = 0;
    dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvResourceDesc.Width = m_width;
    dsvResourceDesc.Height = m_height;
    dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    dsvResourceDesc.SampleDesc.Count = 1;
    dsvResourceDesc.SampleDesc.Quality = 0;
    dsvResourceDesc.MipLevels = 1;
    dsvResourceDesc.DepthOrArraySize = 1;
    dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;


    m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &dsvResourceDesc, D3D12_RESOURCE_STATE_COMMON, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0), IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf()));
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, GetCPUDescriptorHandleForDSV());
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Practice::GetCPUDescriptorHandleForRTV()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Practice::GetCPUDescriptorHandleForDSV() {
    return m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void DX12Practice::LogAdapter(IDXGIFactory4* dxgiFactory)
{
    // Log Adapter
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    while (dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);
        std::wstring text = L"*** Adapter : ";
        text += desc.Description;
        text += L"\n";
        OutputDebugString(text.c_str());
        LogDisplyOutput(adapter);
        adapter->Release();
        ++i;
    }
}

void DX12Practice::LogDisplyOutput(IDXGIAdapter* adapter)
{
    // Log Output
    UINT j = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC outputDesc;
        output->GetDesc(&outputDesc);
        std::wstring outputText = L"**** Display output : ";
        outputText += outputDesc.DeviceName;
        outputText += L"\n";
        OutputDebugString(outputText.c_str());
        LogDisplyModes(output, DXGI_FORMAT_R8G8B8A8_UNORM);
        output->Release();
        ++j;
    }
}

void DX12Practice::LogDisplyModes(IDXGIOutput * output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;
    output->GetDisplayModeList(format, flags, &count, nullptr);
    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& mode : modeList)
    {
        std::wstring text = L" Width : " + std::to_wstring(mode.Width) +
            L" Height : " + std::to_wstring(mode.Height) +
            L" Refresh : " + std::to_wstring(mode.RefreshRate.Numerator / mode.RefreshRate.Denominator) + L"\n";
        OutputDebugString(text.c_str());
    }
}

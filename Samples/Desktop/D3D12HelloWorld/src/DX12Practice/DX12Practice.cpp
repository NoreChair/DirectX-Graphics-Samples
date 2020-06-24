// DX12Practice.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include <memory>
#include <DirectXPackedVector.h>
#include "DX12Practice.h"
#include "D3DUtil.h"

DX12Practice::DX12Practice(UINT width, UINT height, std::wstring name) : DXSample(width, height, name)
{
}

void DX12Practice::OnInit()
{
    CreatePipeline();
    CreateAsset();

    // wait for GPU resource complete upload
    const UINT64 curFenceValue = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), curFenceValue));
    ++m_fenceValue;
    if (m_fence->GetCompletedValue() < curFenceValue) {
        m_fence->SetEventOnCompletion(curFenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    // release uploader resource
    m_vbUploadBuffer = nullptr;
    m_ibUploadBuffer = nullptr;
    m_texture.DisposeLoader();
}


void DX12Practice::OnUpdate()
{

}

void DX12Practice::OnRender()
{
    PopulateCommandList();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    ThrowIfFailed(m_swapChain->Present(1, 0));
    WaitForPreviousFrame();
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
    //LogAdapter(dxgiFactory.Get());
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
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    CreateRenderHeap();
    CreateRenderTargetView();
    CreateDepthStencilView();
    CreateVertexIndexBuffer();
    CreateTextureBuffer();
    CreateRenderPipeline();
}

void DX12Practice::CreateRenderHeap()
{
    // Create Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDesc.NumDescriptors = s_frameCount;
    rtvDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDesc.NumDescriptors = 1;
    dsvDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));
}

void DX12Practice::CreateRenderTargetView()
{
    // Create Render Target View
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < s_frameCount; ++i)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf()));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
        handle.Offset(1, m_rtvDescriptorSize);
    }
}

void DX12Practice::CreateDepthStencilView()
{
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

    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &dsvResourceDesc, D3D12_RESOURCE_STATE_COMMON, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0), IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf()));
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, GetCPUDescriptorHandleForDSV());
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    m_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
}

void DX12Practice::CreateVertexIndexBuffer()
{
    Vertex verties[] = {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f },{ 0.5f, 0.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 1.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f } }
    };

    UINT16 indices[] = {
        0,1,2
    };

    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    UINT vertiesSize = sizeof(verties);
    UINT indicesSize = sizeof(indices);

    m_vertexBuffer = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), (void*)(&verties[0]), vertiesSize, m_vbUploadBuffer);
    m_indexBuffer = D3DUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), (void*)&indices[0], indicesSize, m_ibUploadBuffer);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = vertiesSize;
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indicesSize;

    m_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
}

void DX12Practice::CreateTextureBuffer()
{
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    // init texture
    m_texture.m_width = 256;
    m_texture.m_height = 256;
    m_texture.m_name = "TestTexture";
    m_texture.m_rawData.reset(GenTextureData(256, 256));
    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 256, 256), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_texture.m_textureGPU.GetAddressOf())));

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &CD3DX12_RESOURCE_DESC::Buffer(m_texture.m_width * m_texture.m_height * 4), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_texture.m_textureUploader.GetAddressOf())));

    byte* pData = nullptr;
    m_texture.m_textureUploader->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    memcpy(pData, m_texture.m_rawData.get(), m_texture.m_width * m_texture.m_height * 4);
    m_texture.m_textureUploader->Unmap(0, nullptr);

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = m_texture.m_rawData.get();
    textureData.RowPitch = m_texture.m_width * 4;
    textureData.SlicePitch = textureData.RowPitch * m_texture.m_height;

    UpdateSubresources(m_commandList.Get(), m_texture.m_textureGPU.Get(), m_texture.m_textureUploader.Get(), 0, 0, 1, &textureData);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.m_textureGPU.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    m_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

    // create texture buffer view
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        0
    };
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_srvHeap.GetAddressOf())));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    m_texture.m_srvHeap = m_srvHeap;
    m_texture.m_indexInHeap = 0;
    m_device->CreateShaderResourceView(m_texture.m_textureGPU.Get(), &srvDesc, m_texture.GetCPUHandle(m_srvDescriptorSize));

#ifndef STATIC_SAMPLER
    // create sampler descriptor heap
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_samplerHeap.GetAddressOf())));

    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    m_sampler.m_indexInHeap = 0;
    m_sampler.m_samplerHeap = m_samplerHeap;
    m_device->CreateSampler(&samplerDesc, m_sampler.GetCPUHandle(m_samplerDescriptorSize));
#endif // !STATIC_SAMPLER

}

void DX12Practice::CreateRenderPipeline()
{
    DirectX::XMStoreFloat4x4(&m_cbuffer.worldViewProj, DirectX::XMMatrixIdentity());
    m_cbuffer.color = { 0.0,1.0,1.0,1.0 };
    UINT cbufferSize = D3DUtil::CalcCBufferSize(sizeof(DX12Practice::PreDrawCBuffer));
    m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(cbufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_constBuffer.GetAddressOf()));

    byte* pData = nullptr;
    m_constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    memcpy(pData, &m_cbuffer, sizeof(PreDrawCBuffer));
    m_constBuffer->Unmap(0, nullptr);

    D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        0
    };
    m_device->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf()));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc = {
        m_constBuffer->GetGPUVirtualAddress(),
        D3DUtil::CalcCBufferSize(sizeof(PreDrawCBuffer))
    };
    m_device->CreateConstantBufferView(&cbViewDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

    // root signature
#ifndef STATIC_SAMPLER
    const int paramCount = 3;
#else
    const int paramCount = 2;
#endif

    CD3DX12_ROOT_PARAMETER slotRootParamter[paramCount];
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParamter[0].InitAsDescriptorTable(1, &cbvTable);
    slotRootParamter[1].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

#ifndef STATIC_SAMPLER
    CD3DX12_DESCRIPTOR_RANGE samplerTable;
    samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    slotRootParamter[2].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC signature_desc(paramCount, slotRootParamter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
#else
    CD3DX12_STATIC_SAMPLER_DESC staticSamplerDesc[1];
    staticSamplerDesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP); // linear warp
    CD3DX12_ROOT_SIGNATURE_DESC signature_desc(paramCount, slotRootParamter, _countof(staticSamplerDesc), staticSamplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
#endif

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())));

    // compile shader
    ComPtr<ID3DBlob> vsByteCode;
    ComPtr<ID3DBlob> fsByteCode;
    vsByteCode = D3DUtil::CompileShader(L"Assets/SimpleUnlit.hlsl", nullptr, "VS", "vs_5_1");
    fsByteCode = D3DUtil::CompileShader(L"Assets/SimpleUnlit.hlsl", nullptr, "PS", "ps_5_1");

    // rasterizer
    CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);
    rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rsDesc.CullMode = D3D12_CULL_MODE_NONE;
    rsDesc.FrontCounterClockwise = false;

    // input layout
    D3D12_INPUT_ELEMENT_DESC elementDesc[] = {
    {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
    {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { elementDesc,_countof(elementDesc) };

    // graphic pipeline object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.pRootSignature = m_rootSignature.Get();
    pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vsByteCode.Get());
    pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(fsByteCode.Get());
    pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineStateDesc.SampleMask = UINT_MAX;
    pipelineStateDesc.RasterizerState = rsDesc;
    pipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pipelineStateDesc.InputLayout = inputLayoutDesc;
    pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pipelineStateDesc.SampleDesc.Count = 1;
    pipelineStateDesc.SampleDesc.Quality = 0;

    m_device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(m_graphicState.GetAddressOf()));
}

void DX12Practice::CreateComputePipeline()
{
    CD3DX12_ROOT_PARAMETER param[1];
    param[0].InitAsConstantBufferView(0);

    CD3DX12_ROOT_SIGNATURE_DESC signature_desc(_countof(param), param);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    m_device->CreateRootSignature(0, nullptr, 0, IID_PPV_ARGS(m_computeSignature.GetAddressOf()));
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};

    ThrowIfFailed(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_computeState.GetAddressOf())));
}

byte* DX12Practice::GenTextureData(UINT width, UINT height)
{
    const UINT rowPitch = width * 4;
    const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const UINT cellHeight = width >> 3;    // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * height;

    byte* data = new byte[textureSize];

    for (UINT n = 0; n < textureSize; n += 4)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            data[n] = 0x00;        // R
            data[n + 1] = 0x00;    // G
            data[n + 2] = 0x00;    // B
            data[n + 3] = 0xff;    // A
        }
        else
        {
            data[n] = 0xff;        // R
            data[n + 1] = 0xff;    // G
            data[n + 2] = 0xff;    // B
            data[n + 3] = 0xff;    // A
        }
    }

    return data;
}

void DX12Practice::PopulateCommandList()
{
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_graphicState.Get()));

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandleForRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetCPUDescriptorHandleForDSV();

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MinDepth = D3D12_MIN_DEPTH;
    viewport.MaxDepth = D3D12_MAX_DEPTH;
    D3D12_RECT scissortRect = { 0,0,(long)m_width,(long)m_height };

    XMFLOAT4 color = { 0.0f,0.2f,0.4f,1.0f };

    m_commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    m_commandList->ClearRenderTargetView(rtvHandle, &color.x, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, D3D12_MAX_DEPTH, 0, 0, nullptr);
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissortRect);
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppcbvHeap[1] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, ppcbvHeap);
    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

    ID3D12DescriptorHeap* ppsrvHeap[1] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, ppsrvHeap);
    m_commandList->SetGraphicsRootDescriptorTable(1, m_texture.GetGPUHandle(m_srvDescriptorSize));

#ifndef STATIC_SAMPLER
    ID3D12DescriptorHeap* ppsamplerHeap[1] = { m_samplerHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, ppsamplerHeap);
    m_commandList->SetGraphicsRootDescriptorTable(2, m_sampler.GetGPUHandle(m_samplerDescriptorSize));
#endif

    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    m_commandList->Close();
}

void DX12Practice::WaitForPreviousFrame()
{
    const UINT64 curFence = m_fenceValue;
    // set synchronized signal
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), curFence));
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < curFence) {
        // if fence not completed last synchronized signal,then wait for completion
        ThrowIfFailed(m_fence->SetEventOnCompletion(curFence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Practice::GetCPUDescriptorHandleForRTV()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Practice::GetCPUDescriptorHandleForDSV() {
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
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

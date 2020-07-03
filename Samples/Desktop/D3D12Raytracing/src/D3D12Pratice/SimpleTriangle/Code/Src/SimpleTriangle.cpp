#include <SimpleTriangle.h>
#include <dxcapi.h>
#include <d3dcompiler.h>

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment)                                         \
  (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

SimpleTriangle::SimpleTriangle(UINT width, UINT height, std::wstring name) :DXSample(width, height, name)
{
}

void SimpleTriangle::OnInit()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, s_frameCount, D3D_FEATURE_LEVEL_11_0, DX::DeviceResources::c_RequireTearingSupport);
    m_deviceResources->RegisterDeviceNotify(this);
    m_deviceResources->SetWindow(Win32Application::GetHwnd(), m_width, m_height);
    m_deviceResources->InitializeDXGIAdapter();

    CreateDeviceResource();
    CreateVertexIndexBuffer();
    CreateAccelerationStructure();
    CreateRootSignature();

    m_deviceResources->WaitForGpu();

    m_bottomLevelAS.pScratch = nullptr;
    m_topLevelAS.pScratch = nullptr;

}

void SimpleTriangle::OnUpdate()
{
}

void SimpleTriangle::OnRender()
{
}

void SimpleTriangle::OnSizeChanged(UINT width, UINT height, bool minimized)
{
}

void SimpleTriangle::OnDestroy()
{
}

void SimpleTriangle::OnDeviceLost()
{
}

void SimpleTriangle::OnDeviceRestored()
{
}

void SimpleTriangle::CreateDeviceResource()
{
    // create device/command list/command allocator/command queue/swap chain/rtv/dsv/fence
    m_deviceResources->CreateDeviceResources();
    ThrowIfFalse(m_deviceResources->GetD3DDevice()->QueryInterface(m_dxrDevice.GetAddressOf()));
    ThrowIfFailed(m_deviceResources->GetCommandList()->QueryInterface(m_commandList.GetAddressOf()));
}

void SimpleTriangle::CreateVertexIndexBuffer()
{
    auto commandList = m_deviceResources->ResetAndGetCommandList();

    Vertex vertices[] = {
        {{0,-0.7f,0.0}},
        {{-0.7f,0.7f,0.0}},
        {{0.7f,0.7f,0.0}}
    };

    Index indices[] = {
        0,1,2
    };

    GpuUploadBuffer vertexUploader;
    GpuUploadBuffer indexUploader;

    UINT vertexByteSize = sizeof(vertices);
    UINT indexByteSize = sizeof(indices);

    vertexUploader.Allocate(m_dxrDevice.Get(), vertexByteSize, L"VertexBufferUploder");
    auto pData = vertexUploader.MapCpuWriteOnly();
    memcpy(pData, vertices, vertexByteSize);

    indexUploader.Allocate(m_dxrDevice.Get(), indexByteSize, L"IndexBufferUploder");
    pData = indexUploader.MapCpuWriteOnly();
    memcpy(pData, indices, indexByteSize);

    m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexByteSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_vertexBuffer.GetAddressOf()));
    m_vertexBuffer->SetName(L"VertexBuffer");

    m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexByteSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_indexBuffer.GetAddressOf()));
    m_indexBuffer->SetName(L"IndexBuffer");

    D3D12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(vertexUploader.GetResource().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(indexUploader.GetResource().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(_countof(barrier), barrier);

    commandList->CopyResource(m_vertexBuffer.Get(), vertexUploader.GetResource().Get());
    commandList->CopyResource(m_indexBuffer.Get(), indexUploader.GetResource().Get());

    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    commandList->ResourceBarrier(_countof(barrier), barrier);
    m_deviceResources->ExecuteCommandList();
}

void SimpleTriangle::CreateAccelerationStructure()
{
    m_deviceResources->ResetAndGetCommandList();

    // build ray tracing geometry
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.IndexCount = m_indexBuffer->GetDesc().Width / sizeof(Index);
    geometryDesc.Triangles.VertexCount = m_vertexBuffer->GetDesc().Width / sizeof(Vertex);
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Build bottom level acceleration structure
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInput{};
        bottomLevelInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInput.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        bottomLevelInput.NumDescs = 1;
        bottomLevelInput.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        bottomLevelInput.pGeometryDescs = &geometryDesc;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
        m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInput, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = ROUND_UP(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        prebuildInfo.ResultDataMaxSizeInBytes = ROUND_UP(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(m_bottomLevelAS.pResult.GetAddressOf()));
        m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_bottomLevelAS.pScratch.GetAddressOf()));

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
        blasDesc.DestAccelerationStructureData = m_bottomLevelAS.pResult->GetGPUVirtualAddress();
        blasDesc.Inputs = bottomLevelInput;
        blasDesc.SourceAccelerationStructureData = 0;
        blasDesc.ScratchAccelerationStructureData = m_bottomLevelAS.pScratch->GetGPUVirtualAddress();
        m_commandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
    }

    // build top level acceleration structure
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInput{};
        topLevelInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInput.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        topLevelInput.NumDescs = 1;
        topLevelInput.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};

        m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInput, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = ROUND_UP(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        prebuildInfo.ResultDataMaxSizeInBytes = ROUND_UP(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        UINT instanceDescByteSize = ROUND_UP(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 1, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(m_topLevelAS.pResult.GetAddressOf()));
        m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_topLevelAS.pScratch.GetAddressOf()));
        m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(instanceDescByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_topLevelAS.pInstanceDesc.GetAddressOf()));

        XMMATRIX m = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
        memcpy(instanceDesc.Transform, &m, sizeof(instanceDesc.Transform));
        instanceDesc.InstanceID = 0;
        instanceDesc.InstanceMask = 0xff;
        instanceDesc.InstanceContributionToHitGroupIndex = 0;
        instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDesc.AccelerationStructure = m_bottomLevelAS.pResult->GetGPUVirtualAddress();

        void* pData;
        m_topLevelAS.pInstanceDesc->Map(0, nullptr, &pData);
        memcpy(pData, &instanceDesc, sizeof(instanceDesc));
        m_topLevelAS.pInstanceDesc->Unmap(0, nullptr);

        topLevelInput.InstanceDescs = m_topLevelAS.pInstanceDesc->GetGPUVirtualAddress();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
        tlasDesc.DestAccelerationStructureData = m_topLevelAS.pResult->GetGPUVirtualAddress();
        tlasDesc.Inputs = topLevelInput;
        tlasDesc.SourceAccelerationStructureData = 0;
        tlasDesc.ScratchAccelerationStructureData = m_topLevelAS.pScratch->GetGPUVirtualAddress();
        m_commandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    }

    D3D12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAS.pResult.Get());
    barrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_topLevelAS.pResult.Get());
    m_commandList->ResourceBarrier(2, barrier);
    m_deviceResources->ExecuteCommandList();
}

void SimpleTriangle::CreateRootSignature()
{
    // ray gen
    {
        CD3DX12_ROOT_PARAMETER param[2];
        param[0].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0));
        param[1].InitAsShaderResourceView(0);
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(2, param);
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice, m_rayGenRootSignature);
    }

    // hit group
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr);
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice, m_hitGroupRootSignature);
    }

    // miss
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr);
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice, m_missRootSignature);
    }
}

void SimpleTriangle::CreateRayTracingOutput()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NumDescriptors = 1;
    m_dxrDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_uavHeap.GetAddressOf()));
}

void SimpleTriangle::CreateShaderLibrary()
{

}

void SimpleTriangle::CreateDXRPipeline()
{
}

inline static void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12Device> device, ComPtr<ID3D12RootSignature> rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(rootSig))));
}

inline static IDxcBlob* CompileShaderLibrary(const std::string & shaderStr, const std::wstring& name)
{
    static IDxcCompiler* s_pCompiler = nullptr;
    static IDxcLibrary* s_pLibrary = nullptr;
    static IDxcIncludeHandler* s_pIncludeHandler = nullptr;

    if (s_pCompiler == nullptr) {
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_pCompiler)));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&s_pLibrary)));
        s_pLibrary->CreateIncludeHandler(&s_pIncludeHandler);
    }

    IDxcBlobEncoding* pBlob = nullptr;
    ThrowIfFailed(s_pLibrary->CreateBlobWithEncodingFromPinned(shaderStr.c_str(), (UINT)shaderStr.size(), 0, &pBlob));

    IDxcOperationResult* pResult;
    ThrowIfFailed(s_pCompiler->Compile(pBlob, name.c_str(), L"", L"lib_6_3", nullptr, 0, nullptr, 0, s_pIncludeHandler, &pResult));

    HRESULT resultState;
    ThrowIfFailed(pResult->GetStatus(&resultState));
    if (FAILED(resultState)) {
        IDxcBlobEncoding *pError;
        auto hr = pResult->GetErrorBuffer(&pError);
        if (FAILED(hr)) {
            throw std::logic_error("Failed to get shader compiler error");
        }

        // Convert error blob to a string
        std::vector<char> infoLog(pError->GetBufferSize() + 1);
        memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
        infoLog[pError->GetBufferSize()] = 0;

        std::string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        throw std::logic_error("Failed compile shader");
    }

    IDxcBlob* pResultBlob = nullptr;
    ThrowIfFailed(pResult->GetResult(&pResultBlob));
    return pResultBlob;
}


#include <SimpleTriangle.h>
#include <fstream>
#include <sstream>
#include <dxcapi.h>
#include <RayPayload.h>
#include <DirectXRaytracingHelper.h>

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment)                                         \
  (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

SimpleTriangle::SimpleTriangle(UINT width, UINT height, std::wstring name) :DXSample(width, height, name)
{
}

void SimpleTriangle::OnInit()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, s_frameCount, D3D_FEATURE_LEVEL_11_0, 0, UINT_MAX);
    m_deviceResources->RegisterDeviceNotify(this);
    m_deviceResources->SetWindow(Win32Application::GetHwnd(), m_width, m_height);
    m_deviceResources->InitializeDXGIAdapter();


    ThrowIfFalse(IsDirectXRaytracingSupported(m_deviceResources->GetAdapter()),
        L"ERROR: DirectX Raytracing is not supported by your OS, GPU and/or driver.\n\n");

    CreateDeviceResource();
    CreateVertexIndexBuffer();
    CreateAccelerationStructure();
    CreateRayTracingOutput();
    CreateRootSignature();
    CreateDXRPipeline();
    CreateShaderTables();
    m_deviceResources->WaitForGpu();

    m_vertexUploader.reset(nullptr);
    m_indexUploader.reset(nullptr);
    m_bottomLevelAS.pScratch = nullptr;
    m_topLevelAS.pScratch = nullptr;

}

void SimpleTriangle::OnUpdate()
{
}

void SimpleTriangle::OnRender()
{
    m_deviceResources->Prepare();

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = m_raygenShaderTable->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes = m_raygenShaderTable->GetDesc().Width;
    desc.MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
    desc.MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
    desc.MissShaderTable.StrideInBytes = desc.MissShaderTable.StrideInBytes;
    desc.HitGroupTable.StartAddress = m_hitgroupShaderTable->GetGPUVirtualAddress();
    desc.HitGroupTable.SizeInBytes = m_hitgroupShaderTable->GetDesc().Width;
    desc.HitGroupTable.StrideInBytes = desc.HitGroupTable.SizeInBytes;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Depth = 1;

    std::vector<ID3D12DescriptorHeap *> heaps = { m_uavHeap.Get() };
    m_commandList->SetDescriptorHeaps(1, heaps.data());
    m_commandList->SetPipelineState1(m_dxrPipeline.Get());
    m_commandList->DispatchRays(&desc);

    auto renderTarget = m_deviceResources->GetRenderTarget();

    D3D12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_dxrOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_commandList->ResourceBarrier(2, barrier);

    m_commandList->CopyResource(renderTarget, m_dxrOutput.Get());

    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_dxrOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_commandList->ResourceBarrier(2, barrier);

    m_deviceResources->Present(D3D12_RESOURCE_STATE_PRESENT);
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
    m_deviceResources->CreateWindowSizeDependentResources();

    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();

    ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(m_dxrDevice.GetAddressOf())));
    ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(m_commandList.GetAddressOf())));
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

    UINT vertexByteSize = sizeof(vertices);
    UINT indexByteSize = sizeof(indices);

    m_vertexUploader = std::make_unique<GpuUploadBuffer>();
    m_vertexUploader->Allocate(m_dxrDevice.Get(), vertexByteSize, L"VertexBufferUploder");
    auto pData = m_vertexUploader->MapCpuWriteOnly();
    memcpy(pData, vertices, vertexByteSize);

    m_indexUploader = std::make_unique<GpuUploadBuffer>();
    m_indexUploader->Allocate(m_dxrDevice.Get(), indexByteSize, L"IndexBufferUploder");
    pData = m_indexUploader->MapCpuWriteOnly();
    memcpy(pData, indices, indexByteSize);

    m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexByteSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_vertexBuffer.GetAddressOf()));
    m_vertexBuffer->SetName(L"VertexBuffer");

    m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexByteSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_indexBuffer.GetAddressOf()));
    m_indexBuffer->SetName(L"IndexBuffer");

    commandList->CopyResource(m_vertexBuffer.Get(), m_vertexUploader->GetResource().Get());
    commandList->CopyResource(m_indexBuffer.Get(), m_indexUploader->GetResource().Get());

    D3D12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
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
    geometryDesc.Triangles.IndexCount = (UINT)m_indexBuffer->GetDesc().Width / sizeof(Index);
    geometryDesc.Triangles.VertexCount = (UINT)m_vertexBuffer->GetDesc().Width / sizeof(Vertex);
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

        XMMATRIX m = DirectX::XMMatrixIdentity();
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
        param[0].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 0));
        param[1].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 1));
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(2, param);
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice.Get(), m_rayGenRootSignature);
        m_rayGenRootSignature->SetName(L"RayGenSignature");
    }

    // hit group
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr);
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice.Get(), m_hitGroupRootSignature);
        m_hitGroupRootSignature->SetName(L"HitGroupSignature");
    }

    // miss
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr);
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice.Get(), m_missRootSignature);
        m_missRootSignature->SetName(L"MissSignature");
    }

    // global
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr);
        SerializeAndCreateRaytracingRootSignature(desc, m_dxrDevice.Get(), m_globalDummySignature);
        m_globalDummySignature->SetName(L"GlobalSignature");
    }
}

void SimpleTriangle::CreateRayTracingOutput()
{
    m_csuSize = m_dxrDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NumDescriptors = 2;
    m_dxrDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_uavHeap.GetAddressOf()));

    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_dxrDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_dxrOutput.GetAddressOf()));
    m_dxrOutput->SetName(L"DXR Output");
    auto handle = m_uavHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    m_dxrDevice->CreateUnorderedAccessView(m_dxrOutput.Get(), nullptr, &uavDesc, handle);

    handle.ptr += m_csuSize;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = m_topLevelAS.pResult->GetGPUVirtualAddress();
    m_dxrDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
}

void SimpleTriangle::CreateShaderTables()
{
    ComPtr<ID3D12StateObjectProperties> properties;
    m_dxrPipeline->QueryInterface(IID_PPV_ARGS(&properties));
    UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    // raygen shader table
    {
        void* raygenShaderIdentifier = properties->GetShaderIdentifier(c_rayGenShaderName.c_str());
        ShaderTable raygenShaderTable(m_dxrDevice.Get(), 1, shaderIdentifierSize + 8); //shader identifier and heap pointer
        auto ptr = reinterpret_cast<UINT64*>(m_uavHeap->GetCPUDescriptorHandleForHeapStart().ptr);
        raygenShaderTable.push_back(ShaderRecord(raygenShaderIdentifier, shaderIdentifierSize, &ptr, 8));
        m_raygenShaderTable = raygenShaderTable.GetResource();
    }

    // miss shader table
    {
        void* missShaderIdentifier = properties->GetShaderIdentifier(c_missShaderName.c_str());
        ShaderTable missShaderTable(m_dxrDevice.Get(), 1, shaderIdentifierSize);
        missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
        m_missShaderTable = missShaderTable.GetResource();
    }


    // hitgroup shader table
    {
        void* hitgroupIdentifier = properties->GetShaderIdentifier(c_hitGroupName.c_str());
        ShaderTable hitgroupShaderTable(m_dxrDevice.Get(), 1, shaderIdentifierSize);
        hitgroupShaderTable.push_back(ShaderRecord(hitgroupIdentifier, shaderIdentifierSize));
        m_hitgroupShaderTable = hitgroupShaderTable.GetResource();
    }
}

void SimpleTriangle::CreateDXRPipeline()
{
    //UINT subObjCount =
    //    3 + // DXIL
    //    1 + // HitGroup
    //    2 + // Shader Config And Payload Association
    //    1 + // One Global Signature
    //    3 * 2 + // Three Local Signature And Association
    //    1;// pipeline object

    CD3DX12_STATE_OBJECT_DESC raytracingPipelineDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // DXIL Library
    {
        auto readFileStr = [](std::string path, std::string& shaderStr) {
            std::ifstream fstream;
            fstream.open(path);
            ThrowIfFalse(fstream.is_open());
            std::stringstream sstream;
            sstream << fstream.rdbuf();
            shaderStr = sstream.str();
            fstream.close();
        };

        std::string codeStr;
        readFileStr("Code/Shader/RayGen.hlsl", codeStr);
        auto rayGenLibrary = raytracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        auto code = CompileShaderLibrary(codeStr, L"raygen");
        D3D12_SHADER_BYTECODE rayGenShaderCode = CD3DX12_SHADER_BYTECODE(code->GetBufferPointer(), code->GetBufferSize());
        rayGenLibrary->SetDXILLibrary(&rayGenShaderCode);
        rayGenLibrary->DefineExport(c_rayGenShaderName.c_str());

        readFileStr("Code/Shader/ClosestHit.hlsl", codeStr);
        auto hitGroupLibrary = raytracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        code = CompileShaderLibrary(codeStr, L"hitgroup");
        D3D12_SHADER_BYTECODE hitGroupShaderCode = CD3DX12_SHADER_BYTECODE(code->GetBufferPointer(), code->GetBufferSize());
        hitGroupLibrary->SetDXILLibrary(&hitGroupShaderCode);
        hitGroupLibrary->DefineExport(c_closestHitShaderName.c_str());

        readFileStr("Code/Shader/Miss.hlsl", codeStr);
        auto missLibrary = raytracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        code = CompileShaderLibrary(codeStr, L"miss");
        D3D12_SHADER_BYTECODE missShaderCode = CD3DX12_SHADER_BYTECODE(code->GetBufferPointer(), code->GetBufferSize());
        missLibrary->SetDXILLibrary(&missShaderCode);
        missLibrary->DefineExport(c_missShaderName.c_str());
    }

    // Hit Group
    {
        auto hitGroup = raytracingPipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetClosestHitShaderImport(c_closestHitShaderName.c_str());
        //hitGroup->SetAnyHitShaderImport(c_anyHitShaderName.c_str());
        hitGroup->SetHitGroupExport(c_hitGroupName.c_str());
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    }

    // shader config
    {
        auto shaderConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        UINT maxRayPayloadSize = sizeof(RayPayload);
        UINT maxRayAttrSize = sizeof(RayAttribute);
        shaderConfig->Config(maxRayPayloadSize, maxRayAttrSize);

        auto payloadAssociation = raytracingPipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        payloadAssociation->SetSubobjectToAssociate(*shaderConfig);

        const wchar_t* exports[] = { c_rayGenShaderName.c_str(),c_missShaderName.c_str(),c_closestHitShaderName.c_str() };
        payloadAssociation->AddExports(exports, _countof(exports));
    }

    // global signature
    {
        auto signature = raytracingPipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        signature->SetRootSignature(m_globalDummySignature.Get());
    }

    // local signature
    {
        auto associationSignature = [&](ID3D12RootSignature* rootSignature, std::wstring exportName) {
            auto signature = raytracingPipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            signature->SetRootSignature(rootSignature);
            auto association = raytracingPipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            association->SetSubobjectToAssociate(*signature);
            association->AddExport(exportName.c_str());
        };

        associationSignature(m_rayGenRootSignature.Get(), c_rayGenShaderName);
        associationSignature(m_missRootSignature.Get(), c_missShaderName);
        associationSignature(m_hitGroupRootSignature.Get(), c_hitGroupName);
    }

    // pipeline config
    {
        auto pipelineConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        // only one ray hit
        pipelineConfig->Config(2);
    }

    ThrowIfFailed(m_dxrDevice->CreateStateObject(raytracingPipelineDesc, IID_PPV_ARGS(m_dxrPipeline.GetAddressOf())));
}

void SimpleTriangle::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12Device* device, ComPtr<ID3D12RootSignature>& rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
}

IDxcBlob* SimpleTriangle::CompileShaderLibrary(const std::string & shaderStr, const std::wstring& name)
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


#include "DepthOnlyPass.h"
#include "BufferManager.h"
#include "CompiledShaders/DepthViewerVS.h"
#include "CompiledShaders/DepthViewerPS.h"

DepthOnlyPass::DepthOnlyPass() : m_depthOnlyRS(), m_depthOnlyPSO(), m_depthOnlyCutoutPSO() {}

void DepthOnlyPass::Setup() {
    m_depthOnlyRS.Reset(2, 1);
    m_depthOnlyRS.InitStaticSampler(0, Graphics::SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
    m_depthOnlyRS[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    //m_depthOnlyRS[1].InitAsDescriptorTable(2, D3D12_SHADER_VISIBILITY_PIXEL);
    //m_depthOnlyRS[1].SetTableRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    //m_depthOnlyRS[1].SetTableRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
    m_depthOnlyRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    DXGI_FORMAT depthFormat = Graphics::g_SceneDepthBuffer.GetFormat();
    m_depthOnlyPSO.SetRootSignature(m_depthOnlyRS);
    m_depthOnlyPSO.SetRasterizerState(Graphics::RasterizerDefault);
    m_depthOnlyPSO.SetBlendState(Graphics::BlendNoColorWrite);
    m_depthOnlyPSO.SetDepthStencilState(Graphics::DepthStateReadWrite);
    m_depthOnlyPSO.SetInputLayout(Graphics::InputLayoutDefault.NumElements, Graphics::InputLayoutDefault.pInputElementDescs);
    m_depthOnlyPSO.SetRenderTargetFormats(0, nullptr, depthFormat);
    m_depthOnlyPSO.SetVertexShader(g_pDepthViewerVS, sizeof(g_pDepthViewerVS));
    m_depthOnlyPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_depthOnlyPSO.Finalize();

    m_depthOnlyCutoutPSO = m_depthOnlyCutoutPSO;
    m_depthOnlyPSO.SetRasterizerState(Graphics::RasterizerTwoSided);
    m_depthOnlyCutoutPSO.SetPixelShader(g_pDepthViewerPS, sizeof(g_pDepthViewerPS));
    m_depthOnlyCutoutPSO.Finalize();
}

void DepthOnlyPass::Execute(const Math::Camera& camera, GraphicsContext* pContext) {
    struct DepthOnlyConstants {
        Math::Matrix4 m_viewProjMatrix;
    };
    // TODO : use DynamicUploderBuffer and GPUBuffer instead of per frame create/destroy dynamic buffer
    DepthOnlyConstants constantBuffer;
    constantBuffer.m_viewProjMatrix = camera.GetViewProjMatrix();

    pContext->SetRootSignature(m_depthOnlyRS);
    pContext->SetPipelineState(m_depthOnlyPSO);
    pContext->SetDynamicConstantBufferView(0, sizeof(DepthOnlyConstants), &constantBuffer);
    pContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DrawObjects(pContext, Model::EMaterialType::Cutout);

    pContext->SetPipelineState(m_depthOnlyCutoutPSO);
    DrawObjects(pContext, Model::EMaterialType::Cutout);
}

void DepthOnlyPass::Dispose() {

}

void DepthOnlyPass::DrawObjects(GraphicsContext * pContext, Model::EMaterialType filter) {
    for (auto iter = m_modelList.cbegin(); iter != m_modelList.cend(); iter++) {
        const Model* pModel = *iter;
        auto indexBufferView = pModel->m_IndexBuffer.IndexBufferView();
        pContext->SetIndexBuffer(indexBufferView);
        auto vertexBufferView = pModel->m_VertexBuffer.VertexBufferView();
        pContext->SetVertexBuffer(0, vertexBufferView);
        DrawObject(pContext, pModel, filter);
    }
}

void DepthOnlyPass::DrawObject(GraphicsContext* pContext, const Model* pModel, Model::EMaterialType filter) {
    uint32_t materialIdx = 0xFFFFFFFFul;
    for (UINT i = 0; i < pModel->m_Header.meshCount; i++) {
        const auto& subMesh = pModel->m_pMesh[i];
        if (materialIdx != subMesh.materialIndex) {
            if (filter != pModel->m_pMaterialType[subMesh.materialIndex]) {
                continue;
            }
        }

        materialIdx = subMesh.materialIndex;
        if (filter == Model::EMaterialType::Cutout) {
            auto handle = pModel->GetSRV(materialIdx, Model::ETextureType::Diffuse);
            pContext->SetDynamicDescriptor(0, 0, handle);
        }

        int indexCount = 0;
        int indexStartLocation = 0;
        int vertexStartLocation = 0;
        pModel->GetMeshDrawInfo(i, indexCount, indexStartLocation, vertexStartLocation);
        pContext->DrawIndexed(indexCount, indexStartLocation, vertexStartLocation);
    }
}

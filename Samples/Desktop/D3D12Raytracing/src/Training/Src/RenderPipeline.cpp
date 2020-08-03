#include <RenderPipeline.h>
#include <d3dx12.h>
#include <InputLayout.h>

RenderPipeline::RenderPipeline() : m_pInputLayout(nullptr), m_pipelineStateDesc{}, m_graphicsPipelineState(nullptr) {
    m_pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    m_pipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_pipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    m_pipelineStateDesc.SampleDesc.Count = 1;
    m_pipelineStateDesc.SampleDesc.Quality = 0;
    m_pipelineStateDesc.SampleMask = UINT_MAX;
    m_pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
}

RenderPipeline::~RenderPipeline() {

}

void RenderPipeline::SetRootSignature(ID3D12RootSignature * pRootSignature) {
    m_pipelineStateDesc.pRootSignature = pRootSignature;
}

void RenderPipeline::SetShaderCode(ShaderCodeType codeType, D3D12_SHADER_BYTECODE byteCode) {
    switch (codeType) {
    case VS:
        m_pipelineStateDesc.VS = byteCode;
        break;
    case PS:
        m_pipelineStateDesc.PS = byteCode;
        break;
    case GS:
        m_pipelineStateDesc.GS = byteCode;
        break;
    case DS:
        m_pipelineStateDesc.DS = byteCode;
        break;
    case HS:
        m_pipelineStateDesc.HS = byteCode;
        break;
    default:
        throw std::exception("Not implement case" + codeType);
        break;
    }
}

void RenderPipeline::SetBlendState(D3D12_BLEND_DESC blendDesc) {
    m_pipelineStateDesc.BlendState = blendDesc;
}

void RenderPipeline::SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc) {
    m_pipelineStateDesc.RasterizerState = rasterizerDesc;
}

void RenderPipeline::SetRenderTargetViewFormat(UINT count, DXGI_FORMAT* format) {
    m_pipelineStateDesc.NumRenderTargets = count;
    for (UINT i = 0; i < count; i++) {
        m_pipelineStateDesc.RTVFormats[i] = format[i];
    }
}

void RenderPipeline::SetDepthStencilViewFormat(DXGI_FORMAT format) {
    m_pipelineStateDesc.DSVFormat = format;
}

void RenderPipeline::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsDesc) {
    m_pipelineStateDesc.DepthStencilState = dsDesc;
}

void RenderPipeline::SetSampleState(DXGI_SAMPLE_DESC sampleDesc) {
    m_pipelineStateDesc.SampleDesc = sampleDesc;
}

void RenderPipeline::SetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE value) {
    m_pipelineStateDesc.IBStripCutValue = value;
}

InputLayout * RenderPipeline::GetInputLayout() {
    return m_pInputLayout;
}

D3D12_BLEND_DESC RenderPipeline::GetBlendState() {
    return m_pipelineStateDesc.BlendState;
}

D3D12_RASTERIZER_DESC RenderPipeline::GetRasterizerState() {
    return m_pipelineStateDesc.RasterizerState;
}

D3D12_DEPTH_STENCIL_DESC RenderPipeline::GetDepthStencilState() {
    return m_pipelineStateDesc.DepthStencilState;
}

DXGI_SAMPLE_DESC RenderPipeline::GetSampleState() {
    return m_pipelineStateDesc.SampleDesc;
}

void RenderPipeline::CreatePipeline(ID3D12Device * pDevice) {
    m_pipelineStateDesc.InputLayout = m_pInputLayout->GetInputLayout();
    auto ppPipelineState = m_graphicsPipelineState.GetAddressOf();
    pDevice->CreateGraphicsPipelineState(&m_pipelineStateDesc, IID_PPV_ARGS(ppPipelineState));
    m_graphicsPipelineState = (*ppPipelineState);
}

void RenderPipeline::Apply(ID3D12GraphicsCommandList * pCommandList) {
    pCommandList->SetPipelineState(m_graphicsPipelineState.Get());
    // TODO :
    pCommandList->SetGraphicsRootSignature(m_pipelineStateDesc.pRootSignature);
    m_pInputLayout->Apply(pCommandList);
}

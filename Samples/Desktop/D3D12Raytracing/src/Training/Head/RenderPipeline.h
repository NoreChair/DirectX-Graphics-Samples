#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class InputLayout;

enum ShaderCodeType {
    VS,
    PS,
    GS,
    DS,
    HS,
    CS,
    RT // Ray Tracing
};

class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    void SetRootSignature(ID3D12RootSignature* pRootSignature);
    void SetShaderCode(ShaderCodeType codeType, D3D12_SHADER_BYTECODE byteCode);
    void SetBlendState(D3D12_BLEND_DESC blendDesc);
    void SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc);
    void SetRenderTargetViewFormat(UINT count, DXGI_FORMAT format[]);
    void SetDepthStencilViewFormat(DXGI_FORMAT format);
    void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsDesc);
    void SetSampleState(DXGI_SAMPLE_DESC sampleDesc);
    void SetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE value);

    InputLayout* GetInputLayout();
    D3D12_BLEND_DESC GetBlendState();
    D3D12_RASTERIZER_DESC GetRasterizerState();
    D3D12_DEPTH_STENCIL_DESC GetDepthStencilState();
    DXGI_SAMPLE_DESC GetSampleState();

    void CreatePipeline(ID3D12Device* pDevice);
    void Apply(ID3D12GraphicsCommandList* pCommandList);

private:
    InputLayout* m_pInputLayout;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipelineStateDesc;
    ComPtr<ID3D12PipelineState> m_graphicsPipelineState;
};
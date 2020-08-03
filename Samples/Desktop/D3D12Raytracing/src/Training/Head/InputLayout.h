#pragma once
#include <vector>
#include <d3d12.h>

enum InputType {
    Position,
    Normal,
    Color,
    Texcoord2,
    Texcoord3,
    Texcoord4
};

class InputLayout {
    const std::string c_position = "POSITION";
    const std::string c_normal = "NORMAL";
    const std::string c_color = "COLOR";
    const std::string c_texcoord = "TEXCOORD";
public:
    InputLayout();
    ~InputLayout();

    void AddInputElement(InputType type, UINT index = 0);
    void AddDefaultInputElement();
    void SetVertexBuffer(ID3D12Resource* pBuffer);
    void SetIndexBuffer(ID3D12Resource* pBuffer, bool bit32);
    void Apply(ID3D12GraphicsCommandList* pCommandList);
    D3D12_INPUT_LAYOUT_DESC GetInputLayout();
private:
    UINT alignedByteOffset;
    D3D12_VERTEX_BUFFER_VIEW m_vertexView;
    D3D12_INDEX_BUFFER_VIEW m_indexView;
    D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveType;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElement;
};

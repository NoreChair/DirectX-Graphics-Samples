#include <InputLayout.h>
#include <DXSampleHelper.h>

InputLayout::InputLayout() : alignedByteOffset(0), m_vertexView(), m_indexView(), m_inputLayoutDesc(), m_primitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST), m_inputElement() {

}

InputLayout::~InputLayout() {}

void InputLayout::AddInputElement(InputType type, UINT index = 0) {
    switch (type) {
    case InputType::Position:
        m_inputElement.push_back({ c_position.c_str(), index, DXGI_FORMAT_R32G32B32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        alignedByteOffset += 12;
        break;
    case InputType::Normal:
        m_inputElement.push_back({ c_normal.c_str(), index, DXGI_FORMAT_R32G32B32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,0 });
        alignedByteOffset += 12;
        break;
    case InputType::Color:
        m_inputElement.push_back({ c_color.c_str(), index, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 });
        alignedByteOffset += 16;
        break;
    case InputType::Texcoord2:
        m_inputElement.push_back({ c_texcoord.c_str(), index, DXGI_FORMAT_R32G32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,0 });
        alignedByteOffset += 8;
        break;
    case InputType::Texcoord3:
        m_inputElement.push_back({ c_texcoord.c_str(), index, DXGI_FORMAT_R32G32B32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,0 });
        alignedByteOffset += 12;
        break;
    case InputType::Texcoord4:
        m_inputElement.push_back({ c_texcoord.c_str(), index, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, alignedByteOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,0 });
        alignedByteOffset += 16;
        break;
    default:
        break;
    }
}

void InputLayout::AddDefaultInputElement() {
    AddInputElement(InputType::Position);
    AddInputElement(InputType::Normal);
    AddInputElement(InputType::Texcoord2);
}

void InputLayout::SetVertexBuffer(ID3D12Resource * pBuffer) {
#if defined(DEBUG) || defined(_DEBUG)
    ThrowIfFalse(pBuffer->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    ThrowIfFalse(pBuffer->GetDesc().Height == 1);
    ThrowIfFalse(pBuffer->GetDesc().Format == DXGI_FORMAT_UNKNOWN);
#endif
    m_vertexView.BufferLocation = pBuffer->GetGPUVirtualAddress();
    m_vertexView.SizeInBytes = pBuffer->GetDesc().Width;
    m_vertexView.StrideInBytes = alignedByteOffset;
}

void InputLayout::SetIndexBuffer(ID3D12Resource * pBuffer, bool bit32) {
#if defined(DEBUG) || defined(_DEBUG)
    ThrowIfFalse(pBuffer->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    ThrowIfFalse(pBuffer->GetDesc().Height == 1);
    ThrowIfFalse(pBuffer->GetDesc().Format == DXGI_FORMAT_UNKNOWN);
#endif

    auto format = pBuffer->GetDesc().Format;
    m_indexView.BufferLocation = pBuffer->GetGPUVirtualAddress();
    m_indexView.SizeInBytes = pBuffer->GetDesc().Width;
    m_indexView.Format = bit32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
}

void InputLayout::Apply(ID3D12GraphicsCommandList* pCommandList) {
    pCommandList->IASetIndexBuffer(&m_indexView);
    pCommandList->IASetVertexBuffers(0, 1, &m_vertexView);
    pCommandList->IASetPrimitiveTopology(m_primitiveType);
}

D3D12_INPUT_LAYOUT_DESC InputLayout::GetInputLayout() {
    D3D12_INPUT_LAYOUT_DESC layoutDesc = {};
    layoutDesc.pInputElementDescs = &m_inputElement[0];
    layoutDesc.NumElements = m_inputElement.size();
    return;
}

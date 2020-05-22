#pragma once

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

class D3DUtil
{
public:
    static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void * data, size_t byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
};

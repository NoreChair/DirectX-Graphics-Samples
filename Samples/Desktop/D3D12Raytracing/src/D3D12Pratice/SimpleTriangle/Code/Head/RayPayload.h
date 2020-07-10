#pragma once
#include <DirectXMath.h>

struct RayPayload
{
    DirectX::XMFLOAT4 color;
};

struct RayAttribute {
    DirectX::XMFLOAT2 barycentric;
};
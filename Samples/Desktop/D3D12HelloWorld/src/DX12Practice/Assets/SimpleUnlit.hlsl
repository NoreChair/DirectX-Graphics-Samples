struct CBPerObject
{
    float4x4 worldViewProj;
    float4 color;
};

CBPerObject preObjBuffer : register(b0);
Texture2D diffuseTex : register(t0);
SamplerState linearWarpSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VS(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), preObjBuffer.worldViewProj);
    result.uv = input.uv;
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = diffuseTex.Sample(linearWarpSampler, input.uv);
    return preObjBuffer.color * texColor;
}

struct RayPayload
{
    float4 color;
};

[shader("miss")]
void miss(inout RayPayload payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = DispatchRaysDimensions().xy;
    float ramp = launchIndex.y / dims.y;

    payload.color = float4(0.0f, 0.2f, 0.7 - ramp * 0.3f, 1.0);
}

struct RayPayload
{
    float4 color;
};

RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> output : register(u0);

[shader("raygeneration")]
void raygen()
{
    RayPayload payload;
    payload.color = float4(0.0, 0.0, 1.0, 1.0);

    uint2 launchIndex = DispatchRaysIndex();
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (launchIndex.xy + 0.5f) / dims.xy;
    d = d * 2.00 - 1.0f;

    RayDesc ray;
    ray.Origin = float4(d.x, d.y, 10, 1);
    ray.Direction = float3(0, 0, -1);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    output[launchIndex] = payload.color;
}
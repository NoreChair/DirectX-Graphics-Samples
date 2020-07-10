
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
    RayDesc ray;
    ray.Origin = float4(0, 0, 0, 1);
    ray.Direction = float3(0, 0, 1);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
    uint2 launchIndex = DispatchRaysIndex();
    output[launchIndex] = payload.color;
}
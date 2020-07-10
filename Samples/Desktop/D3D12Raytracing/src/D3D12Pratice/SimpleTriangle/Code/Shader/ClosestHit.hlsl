struct RayPayload
{
    float4 color;
};

typedef BuiltInTriangleIntersectionAttributes Attribute;

[shader("closesthit")]
void closesthit(inout RayPayload payload : SV_RayPayload, Attribute attr)
{
    float3 baryentrices = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    const float3 red = float3(1.0, 0.0, 0.0);
    const float3 green = float3(0.0, 1.0, 0.0);
    const float3 blur = float3(0.0, 0.0, 1.0);
    float3 c = red * baryentrices + green * baryentrices + blur * baryentrices;
    payload.color = float4(c.rgb, 1.0);
}
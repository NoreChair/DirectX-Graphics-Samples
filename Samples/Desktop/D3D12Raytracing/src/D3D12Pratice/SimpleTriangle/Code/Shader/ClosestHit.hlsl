struct RayPayload
{
    float4 color;
};

typedef BuiltInTriangleIntersectionAttributes Attribute;

[shader("closesthit")]
void closesthit(inout RayPayload payload, Attribute attr)
{
    payload.color = float4(1, 0, 0, 1.0);
}
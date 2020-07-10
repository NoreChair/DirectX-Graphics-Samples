struct RayPayload
{
    float4 color;
};

[shader("miss")]
void miss(inout RayPayload payload : SV_RayPayload)
{
    payload.color = float4(0.2f, 0.2f, 0.2f, 1.0);
}
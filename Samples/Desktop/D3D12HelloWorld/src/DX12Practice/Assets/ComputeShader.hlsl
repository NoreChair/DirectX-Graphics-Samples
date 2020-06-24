struct cbSettings
{
    float time;
    float rate;
    int width;
    int height;
};

cbSettings settings;

RWTexture2D<float4> outputTex;

[numthreads(16, 16, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // is this cell should be light or dim
    int cellWidth = settings.width >> 3;
    int cellHeight = settings.height >> 3;
    int cx = dispatchThreadID.x / cellWidth;
    int cy = dispatchThreadID.y / cellHeight;
    float lambda = abs((cx % 2 + cy % 2) - 1.0f); // cx % 2 == cy % 2 ? 1.0 :0.0f;

    //float integer = floor(settings.time / settings.rate) % 2;
    float remainder = settings.time % settings.rate;
    float cosin = (cos(remainder) + 1.0f) * 0.5f; // (0.0,1.0) cos
    float4 light = cosin;
    float4 dim = 1.0 - cosin;
    outputTex[dispatchThreadID.xy] = lerp(light, dim, float4(lambda, lambda, lambda, lambda)); // avoid switch branch
}
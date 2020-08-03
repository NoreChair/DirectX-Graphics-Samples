// Generate 2D lookup table for skin shading
float4 GenerateSkinLUT(float2 P : POSITION) : COLOR {
  float wrap = 0.2;
  float scatterWidth = 0.3;
  float4 scatterColor = float4(0.15, 0.0, 0.0, 1.0);
  float shininess = 40.0;
  float NdotL = P.x * 2 - 1; // remap from [0, 1] to [-1, 1]
  float NdotH = P.y * 2 - 1;
  float NdotL_wrap = (NdotL + wrap) / (1 + wrap); // wrap lighting
  float diffuse = max(NdotL_wrap, 0.0);
  // add color tint at transition from light to dark
  float scatter = smoothstep(0.0, scatterWidth, NdotL_wrap) *
                  smoothstep(scatterWidth * 2.0, scatterWidth, NdotL_wrap);
  float specular = pow(NdotH, shininess);
  if (NdotL_wrap <= 0)
    specular = 0;
  float4 C;
  C.rgb = diffuse + scatter * scatterColor;
  C.a = specular;
  return C;
}
// Shade skin using lookup table
half3 ShadeSkin(sampler2D skinLUT, half3 N, half3 L, half3 H,
                half3 diffuseColor, half3 specularColor)
    : COLOR {
  half2 s;
  s.x = dot(N, L);
  s.y = dot(N, H);
  half4 light = tex2D(skinLUT, s * 0.5 + 0.5);
  return diffuseColor * light.rgb + specularColor * light.a;
}

----------------------------------------------------------------------------

struct a2v {
  float4 pos : POSITION;
  float3 normal : NORMAL;
  float2 texture : TEXCOORD0;
};
struct v2f {
  float4 hpos : POSITION;
  float2 texcoord : TEXCOORD0;
  float4 col : COLOR0;
};

v2f main(a2v IN, uniform float4x4 lightMatrix) {
  v2f OUT;
  // convert texture coordinates to NDC position [-1, 1]
  OUT.hpos.xy = IN.texture * 2 - 1;
  OUT.hpos.z = 0.0;
  OUT.hpos.w = 1.0;
  // diffuse lighting
  float3 N = normalize(mul((float3x3)lightMatrix, IN.normal));
  float3 L = normalize(-mul(lightMatrix, IN.pos).xyz);
  float diffuse = max(dot(N, L), 0);
  OUT.col = diffuse;
  OUT.texcoord = IN.texture;
  return OUT;
}


 v2f main(float2 tex : TEXCOORD0) { 
    v2f OUT;
    // 7 samples, 2 texel spacing
    OUT.tex0 = tex + float2(-5.5, 0);
    OUT.tex1 = tex + float2(-3.5, 0);
    OUT.tex2 = tex + float2(-1.5, 0);
    OUT.tex3 = tex + float2(0, 0);
    OUT.tex4 = tex + float2(1.5, 0);
    OUT.tex5 = tex + float2(3.5, 0);
    OUT.tex6 = tex + float2(5.5, 0);
    return OUT;
}

 half4 main(v2fConnector v2f,uniform sampler2D lightTex) : COLOR { 
    // weights to blur red channel more than green and blue
    const float4 weight[7] = {     { 0.006, 0.0,  0.0,  0.0 },     { 0.061, 0.0,  0.0,  0.0 },     { 0.242, 0.25, 0.25, 0.0 },     { 0.383, 0.5,  0.5,  0.0 },     { 0.242, 0.25, 0.20, 0.0 },     { 0.061, 0.0,  0.0,  0.0 },     { 0.006, 0.0,  0.0,  0.0 },   };
    half4 a;
    a  = tex2D(lightTex, v2f.tex0) * weight[0];
    a += tex2D(lightTex, v2f.tex1) * weight[1];
    a += tex2D(lightTex, v2f.tex2) * weight[2];
    a += tex2D(lightTex, v2f.tex3) * weight[3];
    a += tex2D(lightTex, v2f.tex4) * weight[4];
    a += tex2D(lightTex, v2f.tex5) * weight[5];
    a += tex2D(lightTex, v2f.tex6) * weight[6];
    return a;
}

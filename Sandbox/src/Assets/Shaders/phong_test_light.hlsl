cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorldViewProjection;
}

Texture2D gDiffuseTexture : register(t0);
SamplerState gDiffuseSampler : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalL : NORMAL;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProjection);
    vout.NormalL = normalize(vin.NormalL);
    vout.Color = vin.Color;
    vout.TexCoord = vin.TexCoord;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    const float3 normal = normalize(pin.NormalL);
    const float3 lightDir = normalize(float3(-0.45f, 0.85f, -0.35f));
    const float3 viewDir = normalize(float3(0.0f, 0.0f, -1.0f));
    const float3 halfDir = normalize(lightDir + viewDir);

    const float ambient = 0.22f;
    const float diffuseAmount = saturate(dot(normal, lightDir));
    const float specularAmount = pow(saturate(dot(normal, halfDir)), 32.0f) * 0.45f;

    float4 texel = gDiffuseTexture.Sample(gDiffuseSampler, pin.TexCoord);
    float3 albedo = texel.rgb * pin.Color;
    float3 lit = albedo * (ambient + diffuseAmount * 0.85f) + specularAmount.xxx;
    return float4(saturate(lit), texel.a);
}

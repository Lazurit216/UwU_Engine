cbuffer cbTransform : register(b0)
{
    row_major float4x4 gWorldViewProjection;
}

Texture2D gDiffuseTexture : register(t0);
SamplerState gDiffuseSampler : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProjection);
    vout.Color = vin.Color;
    vout.TexCoord = vin.TexCoord;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuse = gDiffuseTexture.Sample(gDiffuseSampler, pin.TexCoord);
    return diffuse * float4(pin.Color, 1.0f);
}

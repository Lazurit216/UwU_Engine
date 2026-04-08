struct VertexIn
{
    float3 PosL : POSITION;
    float3 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosH = float4(vin.PosL, 1.0f); // already in NDC for this test
    vout.Color = vin.Color;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(pin.Color, 1.0f);
}
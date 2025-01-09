struct VSInput
{
    float3 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 UV : TEXCOORD;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    output.position = float4(input.Position, 1.0f);
    output.UV = input.UV;
    
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.UV);
}
#include "ShaderStructs.hlsli"

cbuffer externalData : register(b0)
{
    float3 colorTint;
};


Texture2D Particle : register(t0);
SamplerState BasicSampler : register(s0);

// Entry point for this pixel shader
float4 main(VertexToPixel_Particle input) : SV_TARGET
{
	// Sample texture and combine with input color
    float4 color = Particle.Sample(BasicSampler, input.uv) * input.color;
    color.rgb *= colorTint;
    
    return float4(0,0,0,1);
}
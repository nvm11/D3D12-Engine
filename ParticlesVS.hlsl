#include "ShaderStructs.hlsli"

cbuffer externalData : register(b0)
{
    matrix view;
    matrix projection;
    
    float4 startColor;
    float4 endColor;
    
    float currentTime;
    float3 acceleration;
    
    int spriteSheetWidth;
    int spriteSheetHeight;
    float spriteSheetFrameWidth;
    float spriteSheetFrameHeight;
    
    float spriteSheetSpeedScale;
    float startSize;
    float endSize;
    float lifetime;
    
    int constrainYAxis;
};

// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);
// Take in an ID for the vertex
VertexToPixel_Particle main(uint id : SV_VertexID) {
	// Set up output
    VertexToPixel_Particle output;
    
    // Get world position
    matrix wvp = mul(projection, mul(view, world));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

	// Pass other data through
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
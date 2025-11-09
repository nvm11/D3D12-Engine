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

VertexToPixel_Particle main(VertexShaderInput_Particle input) {
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
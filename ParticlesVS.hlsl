#include "ShaderStructs.hlsli"

cbuffer externalData : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
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
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
    return output;
}
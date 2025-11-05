
#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
	matrix worldMatrix;       // World transformation matrix
    matrix worldInverseTranspose;
	matrix viewMatrix;        // View transformation matrix
	matrix projectionMatrix;  // Projection transformation matrix
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// --------------------------------------------------------
VertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
    VertexToPixel output;

	// Get screen pos
    matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
	
	// Translate lighting data to world space
    output.normal = normalize(mul((float3x3) worldInverseTranspose, input.normal));
    output.tangent = normalize(mul((float3x3) worldMatrix, input.tangent));
	// Get vertex world position
    output.worldPos = mul(worldMatrix, float4(input.localPosition, 1.0f)).xyz;
	// Pass through uv data
    output.uv = input.uv;

    return output;
}
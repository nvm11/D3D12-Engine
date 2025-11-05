#include "Lighting.hlsli"
#include "ShaderStructs.hlsli"
//Bit Alignment matters
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float3 cameraPosition;
    int lightCount;
    Light lights[MAX_LIGHTS];
}

// Textures
Texture2D AlbedoTexture : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalMap : register(t3);
// Samplers
SamplerState BasicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    // Normalize vectors
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
	
	// Scale and offset the uv
    input.uv = input.uv * uvScale + uvOffset;

	// Normal mapping
    input.normal = NormalFromMap(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);

	// Surface color with gamma correction
    float3 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv).rgb;
    surfaceColor = pow(surfaceColor.rgb, 2.2) * 2;
	
	// Sample roughness
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    // Sample metalness
    float metalness = MetalMap.Sample(BasicSampler, input.uv).r;
    
    // Get the specular color
    // Metals tint reflections, non metals don't (typically)
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
	
    //Begin lighting calculations
	//include ambient lighting ONCE
    float3 totalLight = float3(0, 0, 0);
	
	//angle the surface is viewed from
    float3 surfaceToCamera = normalize(cameraPosition - input.worldPos);
	
	//apply the total lighting
    totalLight += CalculateTotalLightPBR(lightCount, lights, input.normal, surfaceToCamera, input.worldPos, roughness, metalness, surfaceColor, specularColor, 0.0f);
	
	//return modified color
    return float4(GammaCorrect(totalLight), 1);
    
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
    return float4(GammaCorrect(totalLight), 1);
}
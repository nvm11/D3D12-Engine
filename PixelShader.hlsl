
//Bit Alignment matters
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION;
};

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
    //input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);

	// Surface color with gamma correction
    float4 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv);
    //surfaceColor.rgb = pow(surfaceColor.rgb, 2.2) * 2;
	
	// Sample the other maps
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float metal = MetalMap.Sample(BasicSampler, input.uv).r;
	
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
    return surfaceColor;
}
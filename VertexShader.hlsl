
cbuffer ExternalData : register(b0)
{
	matrix worldMatrix;       // World transformation matrix
    matrix worldInverseTranspose;
	matrix viewMatrix;        // View transformation matrix
	matrix projectionMatrix;  // Projection transformation matrix
}

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;  // XYZ position
	float2 uv				: TEXCOORD;        // Texture coord
	float3 normal           : NORMAL;    // Normal vector
	float3 tangent			: TANGENT;   // Tangent vector
};

// Struct representing the data we're sending down the pipeline
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION;
};

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
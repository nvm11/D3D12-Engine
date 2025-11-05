struct VertexShaderInput_Particle
{
    float3 localPosition : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VertexToPixel_Particle
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

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
    float3 localPosition : POSITION; // XYZ position
    float2 uv : TEXCOORD; // Texture coord
    float3 normal : NORMAL; // Normal vector
    float3 tangent : TANGENT; // Tangent vector
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
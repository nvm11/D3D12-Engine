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
    
    // Get ID information
    uint particleID = id / 4; // 4 verts = 1 particle (quads)
    uint cornerID = id % 4; // the corner of the quad
    
    // Get the particle
    Particle p = ParticleData.Load(particleID);
    
    // Calculate age
    float age = currentTime - p.emitTime;
    float agePercent = age / lifetime;
    
    // Get position based on velocity and acceleration
    float3 pos = acceleration * age * age * 0.5f + p.startVelocity * age + p.startPos;
    
    // Interpolate size
    float size = lerp(startSize, endSize, agePercent);
    
    // Offsets for the 4 corners of a quad - we'll only
	// use one for each vertex, but which one depends
	// on the cornerID above.
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f); // TL
    offsets[1] = float2(+1.0f, +1.0f); // TR
    offsets[2] = float2(+1.0f, -1.0f); // BR
    offsets[3] = float2(-1.0f, -1.0f); // BL
	
	// Handle rotation - get sin/cos and build a rotation matrix
    float s, c, rotation = lerp(p.startRotation, p.endRotation, agePercent);
    sincos(rotation, s, c); // One function to calc both sin and cos
    float2x2 rot =
    {
        c, s,
		-s, c
    };

	// Rotate the offset for this corner and apply size
    float2 rotatedOffset = mul(offsets[cornerID], rot) * size;

	// Offset the position based on the camera's right and up vectors (billboarding)
    pos += float3(view._11, view._12, view._13) * rotatedOffset.x; // RIGHT
    pos += (constrainYAxis ? float3(0, 1, 0) : float3(view._21, view._22, view._23)) * rotatedOffset.y; // UP

	// Calculate output position
    matrix viewProj = mul(projection, view);
    output.screenPosition = mul(viewProj, float4(pos, 1.0f));

	// Sprite sheet animation calculations
    float animPercent = fmod(agePercent * spriteSheetSpeedScale, 1.0f);
    uint ssIndex = (uint) floor(animPercent * (spriteSheetWidth * spriteSheetHeight));

	// Get the U/V indices (basically column & row index across the sprite sheet)
    uint uIndex = ssIndex % spriteSheetWidth;
    uint vIndex = ssIndex / spriteSheetWidth; // Integer division is important here!

	// Convert to a top-left corner in uv space (0-1)
    float u = uIndex / (float) spriteSheetWidth;
    float v = vIndex / (float) spriteSheetHeight;

    float2 uvs[4];
    uvs[0] = float2(u, v); //TL
    uvs[1] = float2(u + spriteSheetFrameWidth, v); //TR
    uvs[2] = float2(u + spriteSheetFrameWidth, v + spriteSheetFrameHeight); //BR
    uvs[3] = float2(u, v + spriteSheetFrameHeight); //BL
	
	// Finalize output
    output.uv = saturate(uvs[cornerID]);
    output.color = lerp(startColor, endColor, agePercent);
    
    return output;
}
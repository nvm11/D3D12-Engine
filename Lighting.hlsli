#ifndef __LIGHTING__
#define __LIGHTING__

//File contains all functions necessary to light a scene

//Types of Lights
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT	   1
#define LIGHT_TYPE_SPOT		   2

//Max number of lights in a scene (power of 2)
#define MAX_LIGHTS 64

#define MAX_SPECULAR_EXPONENT 256.0f

//a constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;
//minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f;
static const float PI = 3.14159265359f;

//see Lights.h for explanation of fields
struct Light
{
    //64 bytes (power of 2 is faster)
    int type;
    float3 direction;
    float range;
    float3 position;
    float intensity;
    float3 color;
    float spotInnerAngle;
    float spotOuterAngle;
    float2 padding;
};

    //calculate diffuse lighting
float DiffuseLight(float3 normal, float3 directionToLight)
{
    //automatically normalizes values
    return saturate(dot(normal, directionToLight)); //return direction light "bounces" off object
}

float PhongSpecularLight(float3 normal, float3 directionToLight, float3 surfaceToCamera, float roughness)
{
    //Calculate necessary vectors
    //reflect the light across the normal (direction light leaves surface)
    float3 r = reflect(-directionToLight, normal);
    
    //Create an exponent value to represent the roughness of the surface
    //high value = shiny, low = dull
    float specularExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    
    //Perform specular calculation
    return pow(max(dot(r, surfaceToCamera), 0.0f), specularExponent);
}
//softens the light
float Attenuate(Light currentLight, float3 worldPos)
{
    float dist = distance(currentLight.position, worldPos);
    float attenuation = saturate(1.0f - (dist * dist / (currentLight.range * currentLight.range)));
    return attenuation * attenuation;
}

//performs all necessary calculations for a directional light
float3 DirectionLight(Light currentLight, float3 normal, float3 surfaceToCamera, float roughness, float3 color) //can pass in a scalar to effect specular light on a per-pixel basis
{
    //get direction from surface to light
    float3 surfaceToLight = -currentLight.direction;
    
    //Perform all lighting calculations (ambient is in main)
    float diffuse = DiffuseLight(normal, surfaceToLight);
    float3 specular = PhongSpecularLight(normal, surfaceToLight, surfaceToCamera, roughness);
    
    //return calculated light
    return (diffuse * color + specular) * currentLight.intensity * currentLight.color;
}
//performs all necessary calculations for a point light
float3 PointLight(Light currentLight, float3 normal, float3 surfaceToCamera, float3 worldPosition, float roughness, float3 color)
{
    //get distance from light
    float3 surfaceToLight = normalize(currentLight.position - worldPosition);
    
    //Preform all lighting calculations
    float diffuse = DiffuseLight(normal, surfaceToLight);
    float specular = PhongSpecularLight(normal, surfaceToLight, surfaceToCamera, roughness);
    float attenuation = Attenuate(currentLight, worldPosition);
    
    //return lighting results
    return (diffuse * color + specular) * attenuation * currentLight.intensity * currentLight.color;

}
//performs all necessary calculations for a spotlight
float3 SpotLight(Light currentLight, float3 normal, float3 surfaceToCamera, float3 worldPosition, float roughness, float3 color)
{
    //get necessary components
    float3 surfaceToLight = normalize(currentLight.position - worldPosition);
    float3 angle = saturate(dot(-surfaceToLight, currentLight.direction));
    
    //get two "cones"
    float cosInner = cos(currentLight.spotInnerAngle);
    float cosOuter = cos(currentLight.spotOuterAngle);
    float falloffRange = cosOuter - cosInner;
    
    //simulate shrinking of the lights to form cone
    float spotTerm = saturate((cosOuter - angle) / falloffRange).x;
    
    //use terms with point light calculation to create the cone
    return PointLight(currentLight, normal, surfaceToCamera, worldPosition, roughness, color) * spotTerm;
}

float3 CalculateTotalLight(int numLights, Light lights[MAX_LIGHTS], float3 normal, float3 surfaceToCamera, float3 worldPos, float roughness, float3 color)
{
    
    float3 totalLight; //store total lighting
    //loop through lights
    for (int i = 0; i < numLights; i++)
    {
        //grab a copy of the current index
        Light currentLight = lights[i];
        //normalize the direction to ensure consistent results
        currentLight.direction = normalize(currentLight.direction);
        
        switch (currentLight.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalLight += DirectionLight(currentLight, normal, surfaceToCamera, roughness, color);
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLight(currentLight, normal, surfaceToCamera, worldPos, roughness, color);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLight(currentLight, normal, surfaceToCamera, worldPos, roughness, color);
                break;
        }
    }
    
    return totalLight;
}

float3x3 CreateTBN(float3 normal, float3 tangent)
{
    //build the tbn matrix
	//t = normalized tangent along u
	//b = bitangent (cross of t and n) along v
	//n = normalized surface normal
    float3 t = normalize(tangent - normal * dot(tangent, normal));
    return float3x3(t, cross(t, normal), normal);
}

float3 NormalFromMap(Texture2D normalMap, SamplerState sample, float2 uv, float3 normal, float3 tangent)
{
    //sample the normal map and "unpack" result
    float3 normalMapData = normalize(normalMap.Sample(sample, uv).rgb * 2.0f - 1.0f);
    
    float3x3 tbn = CreateTBN(normal, tangent);
    
    //put normal map value in world space using tbn
    return normalize(mul(normalMapData, tbn));
}

float FresnelReflection(float3 view, float3 surfaceNormal, float specularValue)
{
    //uses Schilick's aproxximation of the Fresnel term:
    //F(n, v, f0) = f0 + (1 - f0)(1 - (n dot v))^5
    //N == normal vector
    //V = view vector
    //f0 = Specular value (usually 0.0f for non metals)
    
    //dot product the normal with the view
    float NdotV = saturate(dot(surfaceNormal, view)); //makes light different at glancing angles
    return specularValue + (1 - specularValue) * pow(1 - NdotV, 5); //return reflection strength
}

float3 ApplyFresnelReflection(float3 cameraPos, float3 worldPos, float3 surfaceNormal, float3 totalLight, SamplerState BasicSampler, TextureCube Skybox, float specularValue)
{
    //get view vector
    float3 view = normalize(cameraPos - worldPos);
    //get reflection of light (camera to pixel vector)
    float3 reflection = reflect(-view, surfaceNormal);
    //combine with skybox color, surface color, and reflection strength
    return lerp(totalLight, Skybox.Sample(BasicSampler, reflection).rgb, FresnelReflection(view, surfaceNormal, specularValue));
}

float3 GammaCorrect(float3 totalLight)
{
    //simply raise the total light to the 1/2.2 power
    return pow(totalLight, 1.0f / 2.2f);

}

//lambert diffuse BRDF (same as the basic lighting diffuse calculation)
//this function assumes the vectors are already normalized
float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

//calculates diffuse amount based on energy conservation
// diffuse - diffuse amount
// F - Fresnel result from microfacet BRDF
// metalness - surface metalness amount
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// normal distribution function: GGX (Trowbridge-Reitz)
// a - Roughness
// h - Half vector: (V + L)/2
// n - Normal
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
    //necessary vectors
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    //apply AFTER squaring (remapping) roughness
    float a2 = max(a * a, MIN_ROUGHNESS);
    //((n dot h)^2 * (a^2 - 1) + 1)
    //can go to zero if roughness is 0 and NdotH is 1; MIN_ROUGHNESS helps here
    float denomToSquare = NdotH2 * (a2 - 1) + 1;
// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// v - view vector
// h - half vector
// f0 - value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    float VdotH = saturate(dot(v, h));
    //final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

//numerator should be NdotV (or NdotL depending on parameters)
//these are also in the BRDF's denominator, so they'll cancel
//leaving them out here AND in the BRDF function because the
//dot products can get very small and cause rounding errors
    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
// Other vectors
    float3 h = normalize(v + l); // That’s an L, not a 1! Careful copy/pasting from a PDF!
// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
// Pass F out of the function for diffuse balance
    F_out = F;
// Final specular formula
// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
// canceled out by our G() term. As such, they have been removed
// from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;
// One last non-obvious requirement: According to the rendering equation,
// specular must have the same NdotL applied as diffuse! We'll apply
// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

//performs all necessary calculations for a direction light (PBR variant)
float3 DirectionLightPBR(Light currentLight, float3 normal, float3 surfaceToCamera, float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
    //get direction from surface to light
    float3 surfaceToLight = -currentLight.direction;
    
    //perform all lighting calculations
    float diffuse = DiffusePBR(normal, surfaceToLight);
    float3 F;
    //this needs specular color (metal or not)
    float3 specular = MicrofacetBRDF(normal, surfaceToLight, surfaceToCamera, roughness, specularColor, F);
    
    //calculate diffused light with energy conservation
    float3 balancedDiffuse = DiffuseEnergyConserve(diffuse, specular, metalness);
    //return calculated light
    //this needs surface color
    return (balancedDiffuse * surfaceColor + specular) * currentLight.intensity * currentLight.color;
}

//performs all necessary calculations for a point light (PBR variant)
float3 PointLightPBR(Light currentLight, float3 normal, float3 surfaceToCamera, float3 worldPosition, float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
    //get distance from light
    float3 surfaceToLight = normalize(currentLight.position - worldPosition);
    
    //Preform all lighting calculations
    float diffuse = DiffusePBR(normal, surfaceToLight);
    float3 F;
    float3 specular = MicrofacetBRDF(normal, surfaceToLight, surfaceToCamera, roughness, specularColor, F);
    float attenuation = Attenuate(currentLight, worldPosition);
    
    //energy conservation
    float3 balancedDiffuse = DiffuseEnergyConserve(diffuse, specular, metalness);
    
    //return lighting results
    return (balancedDiffuse * surfaceColor + specular) * attenuation * currentLight.intensity * currentLight.color;
}

//performs all necessary calculations for a spotlight (PBR variant)
float3 SpotLightPBR(Light currentLight, float3 normal, float3 surfaceToCamera, float3 worldPosition, float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
    //get necessary components
    float3 surfaceToLight = normalize(currentLight.position - worldPosition);
    float3 angle = saturate(dot(-surfaceToLight, currentLight.direction));
    
    //get two "cones"
    float cosInner = cos(currentLight.spotInnerAngle);
    float cosOuter = cos(currentLight.spotOuterAngle);
    float falloffRange = cosOuter - cosInner;
    
    //simulate shrinking of the lights to form cone
    float spotTerm = saturate((cosOuter - angle) / falloffRange).x;
    
    //use terms with point light calculation to create the cone
    return PointLightPBR(currentLight, normal, surfaceToCamera, worldPosition, roughness, metalness, surfaceColor, specularColor) * spotTerm;
}

float3 CalculateTotalLightPBR(int numLights, Light lights[MAX_LIGHTS], float3 normal, float3 surfaceToCamera, float3 worldPos, float roughness, float metalness, float3 surfaceColor, float3 specularColor, float shadowAmount)
{
    
    float3 totalLight; //store total lighting
    //loop through lights
    for (int i = 0; i < numLights; i++)
    {
        //grab a copy of the current index
        Light currentLight = lights[i];
        //normalize the direction to ensure consistent results
        currentLight.direction = normalize(currentLight.direction);
        
        switch (currentLight.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                float3 directionLight = DirectionLightPBR(currentLight, normal, surfaceToCamera, roughness, metalness, surfaceColor, specularColor);
                // TODO: implement shadow logic
                //if (i == 0)
                //{
                    //totalLight += directionLight * shadowAmount;
                //}
                //else
                //{
                    totalLight += directionLight;

                //}
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLightPBR(currentLight, normal, surfaceToCamera, worldPos, roughness, metalness, surfaceColor, specularColor);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLightPBR(currentLight, normal, surfaceToCamera, worldPos, roughness, metalness, surfaceColor, specularColor);
                break;
        }
    }
    
    return totalLight;
}

float2 GetParallaxUV(Texture2D HeightMap, SamplerState BasicSampler, float2 uv, float3 view, float3x3 TBN, int samples, float scale)
{
// Get tangent space view vector
// Note: Multiplying in opposite order is effectively
// transposing the matrix, which acts like an
// invert on a pure 3x3 rotation matrix!
    float3 view_TS = mul(TBN, view); // World to Tangent space - NOTE: B must be negated first!
// Calculate the ray direction, including proper length based on scale
    float viewLength = length(view_TS);
    float parallaxLength = sqrt(viewLength * viewLength - view_TS.z * view_TS.z) / view_TS.z;
    float2 rayDir = normalize(view_TS.xy) * parallaxLength * scale;
// Tracking height and position during raymarch
    float currentHeight = 1.0f;
    float2 currentPos = uv;
    float stepSize = 1.0f / samples;
    float2 uvStep = rayDir * stepSize;
// Calculate uv derivates to support texture sampling
// in a loop with variable iterations
    float2 dx = ddx(uv);
    float2 dy = ddy(uv);
// Raymarch through surface
    [loop]
    for (int i = 0; i < samples; i++)
    {
// Offset along ray and grab the height there
        currentPos -= uvStep;
        currentHeight -= stepSize;
        float heightAtPos = HeightMap.SampleGrad(BasicSampler, currentPos, dx, dy).r;
// If we've gone "below" the heightmap, we've hit!
        if (currentHeight < heightAtPos)
        {
            break;
        }
    }
    return currentPos;
}

#endif
#pragma once
#include <DirectXMath.h>
#include "Light.h"

struct VertexShaderExternalData {
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInverseTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct PixelShaderExternalData {
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
};

// Overall scene data for raytracing
struct RaytracingSceneData
{
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
	float pad;
};

// All material data for raytracing
struct RaytracingMaterial
{
	DirectX::XMFLOAT3 color;
	float roughness;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	float metal;
	DirectX::XMFLOAT3 padding;

	unsigned int albedoIndex;
	unsigned int normalMapIndex;
	unsigned int roughnessIndex;
	unsigned int metalnessIndex;
};

// Per-entity data for raytracing
#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData
{
	RaytracingMaterial materials[MAX_INSTANCES_PER_BLAS];
};

struct ParticleExternalData
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;

	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;

	float currentTime;
	DirectX::XMFLOAT3 acceleration;

	int spriteSheetWidth;
	int spriteSheetHeight;
	float spriteSheetFrameWidth;
	float spriteSheetFrameHeight;

	float spriteSheetSpeedScale;
	float startSize;
	float endSize;
	float lifetime;

	int constrainYAxis;
	DirectX::XMFLOAT3 colorTint;
};

struct RefractiveExternalData
{
	DirectX::XMFLOAT3 clearColor;
	int lightCount;
	DirectX::XMFLOAT3 cameraPosition;
	float refractionScale;

	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;

	float screenWidth;
	float screenHeight;
	int useRefractionSilhouette;
	float pad;

	Light lights[MAX_LIGHTS];
};
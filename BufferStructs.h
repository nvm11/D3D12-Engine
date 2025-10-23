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

// Per-entity data for raytracing
#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData
{
	DirectX::XMFLOAT4 color[MAX_INSTANCES_PER_BLAS];
	unsigned int recursionDepth;
	unsigned int rayPerPixelIndex;
};
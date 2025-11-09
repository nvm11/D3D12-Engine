#pragma once
#include <DirectXMath.h>
struct Particle
{
	float EmitTime;
	DirectX::XMFLOAT3 StartPosition;
	DirectX::XMFLOAT3 StartVelocity;
	float StartRotation;
	float EndRotation;
	DirectX::XMFLOAT3 padding;
};
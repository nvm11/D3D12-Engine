#pragma once
#include "Particle.h"

class Emitter
{
private:
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	int maxParticles;
	Particle* particles;
	int maxParticles;
	int particlesPerSecond;
	DirectX::XMFLOAT3 emitterPos;
	DirectX::XMFLOAT3 positionRandomRange;

public:
	Emitter(const DirectX::XMFLOAT4 startColor,
			const DirectX::XMFLOAT4 endColor,
			const int maxParticles = 100, 
			const int particlesPerSecond,
			const float lifetime = 1.0f,
			const DirectX::XMFLOAT3 emitterPos = DirectX::XMFLOAT3(),
			const DirectX::XMFLOAT3 positionRandomRange = DirectX::XMFLOAT3());
	~Emitter();
};


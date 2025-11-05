#include "Emitter.h"


Emitter::Emitter(const DirectX::XMFLOAT4 startColor, 
				 const DirectX::XMFLOAT4 endColor, 
				 const int maxParticles, 
				 const int particlesPerSecond, const float lifetime, 
				 const DirectX::XMFLOAT3 emitterPos, 
				 const DirectX::XMFLOAT3 positionRandomRange)
	:maxParticles(maxParticles),
	 particlesPerSecond(particlesPerSecond),
	 startColor(startColor),
	 endColor(endColor),
	 emitterPos(emitterPos)
{
	// TODO: Initialize particles
	// TODO: Assign emitter start pos
}

void Emitter::Update(float deltaTime)
{
	// Updatte particles
	// Move information relating to dead/alive particles
}

void Emitter::Draw(std::shared_ptr<Camera> cam)
{
	// Draw particles relative to camera
}

Emitter::~Emitter()
{
	delete[] particles;
}

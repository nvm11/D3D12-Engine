#include "Emitter.h"


void Emitter::UpdateParticle(float deltaTime, int particleIndex)
{
	// Check if particle is dead
	if(particles[particleIndex].EmitTime >= maxParticleLifetime) {
		return;
	}

	// Increment emit time
	particles[particleIndex].EmitTime += deltaTime;
	// Just died?
	if (particles[particleIndex].EmitTime >= maxParticleLifetime) {
		aliveIndex++;
		aliveIndex %= maxParticles; // looped data
		aliveParticleCount--;
		return;
	}

	// Lerp color based on age
	float age = particles[particleIndex].EmitTime / maxParticleLifetime;
	DirectX::XMStoreFloat4(
		&particles[particleIndex].Color,
		DirectX::XMVectorLerp(
			DirectX::XMLoadFloat4(&startColor),
			DirectX::XMLoadFloat4(&endColor),
			age));
}

Emitter::Emitter(const DirectX::XMFLOAT4 startColor,
				 const DirectX::XMFLOAT4 endColor, 
				 const int maxParticles, 
				 const int particlesPerSecond, 
				 const float lifetime, 
				 const DirectX::XMFLOAT3 emitterPos, 
				 const DirectX::XMFLOAT3 positionRandomRange)
	:maxParticles(maxParticles),
	 particlesPerSecond(particlesPerSecond),
	 startColor(startColor),
	 endColor(endColor),
	 maxParticleLifetime(lifetime),
	 positionRandomRange(positionRandomRange)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(emitterPos);

	timeSinceLastEmission = 0.0f;
	aliveIndex = 0;
	deadIndex = 0;
	aliveParticleCount = 0;

	// Initialize array of particles
	particles = new Particle[maxParticles];
}

void Emitter::Update(float deltaTime)
{
	// Update particles
	for (int i = 0; i < maxParticles; i++) {
		UpdateParticle(deltaTime, i);
	}
	// Track lifetimes
	
	// Emit Particles
}

void Emitter::Draw(std::shared_ptr<Camera> cam)
{
	// Draw particles relative to camera
}

Emitter::~Emitter()
{
	delete[] particles;
}

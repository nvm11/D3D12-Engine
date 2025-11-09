#include "Emitter.h"

// Macro for random float in range
#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min)

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

void Emitter::CreateParticle()
{
	// Can we initialize?
	if (aliveParticleCount >= maxParticles) {
		timeSinceLastEmission = 0.0f;
		return;
	}

	// Reset first old particle
	particles[deadIndex] = Particle{};
	
	// Assign color
	particles[deadIndex].Color = startColor;

	// Assign start pos with random offset
	particles[deadIndex].Position = DirectX::XMFLOAT3();
	particles[deadIndex].Position.x += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.x;
	particles[deadIndex].Position.y += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.y;
	particles[deadIndex].Position.z += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.z;

	// Wrap indices
	deadIndex++;
	deadIndex %= maxParticles;
	aliveParticleCount++;

	timeSinceLastEmission = 0.0f;
}



Emitter::Emitter(int maxParticles, 
	int particlesPerSecond,
	float lifetime, 
	float startSize, 
	float endSize, 
	bool constrainYAxis, 
	DirectX::XMFLOAT4 startColor, 
	DirectX::XMFLOAT4 endColor, 
	DirectX::XMFLOAT3 startVelocity, 
	DirectX::XMFLOAT3 velocityRandomRange,
	DirectX::XMFLOAT3 emitterPosition, 
	DirectX::XMFLOAT3 positionRandomRange, 
	DirectX::XMFLOAT2 rotationStartMinMax, 
	DirectX::XMFLOAT2 rotationEndMinMax, 
	DirectX::XMFLOAT3 emitterAcceleration, 
	std::shared_ptr<Material> material, 
	unsigned int spriteSheetWidth, 
	unsigned int spriteSheetHeight, 
	float spriteSheetSpeedScale, 
	bool paused, 
	bool visible)
	:material(material),
	maxParticles(maxParticles),
	particlesPerSecond(particlesPerSecond),
	secondsPerparticle(1.0f / particlesPerSecond),
	lifetime(lifetime),
	startSize(startSize),
	endSize(endSize),
	startColor(startColor),
	endColor(endColor),
	constrainYAxis(constrainYAxis),
	positionRandomRange(positionRandomRange),
	velocityRandomRange(velocityRandomRange),
	startVelocity(startVelocity),
	acceleration(emitterAcceleration),
	rotationStartMinMax(rotationStartMinMax),
	rotationEndMinMax(rotationEndMinMax),
	spriteSheetWidth(max(spriteSheetWidth, 1)),
	spriteSheetHeight(max(spriteSheetHeight, 1)),
	spriteSheetFrameWidth(1.0f / spriteSheetWidth),
	spriteSheetFrameHeight(1.0f / spriteSheetHeight),
	spriteSheetSpeedScale(spriteSheetSpeedScale),
	paused(paused),
	visible(visible),
	particles(0),
	totalEmitterTime(0)

{
	transform = std::make_shared<Transform>(emitterPosition);

	// Set up emission stats
	timeSinceLastEmission = 0;
	livingParticleCount = 0;
	aliveIndex = 0;
	deadIndex = 0;

	// Create the array and resources for particles
}

void Emitter::Update(float deltaTime)
{
	// Update particles
	for (int i = 0; i < maxParticles; i++) {
		UpdateParticle(deltaTime, i);
	}
	
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

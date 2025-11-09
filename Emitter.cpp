#include "Emitter.h"
#include "Graphics.h"

// Macro for random float in range
#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min)

void Emitter::InitializeGPUResources()
{
	// Create Structured Buffer for particles
	UINT bufferSize = sizeof(Particle) * maxParticles;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = Graphics::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&particleBuffer)
	);
}

void Emitter::UpdateParticle(int particleIndex)
{
	// Get age
	float age = totalEmitterTime - particles[particleIndex].EmitTime;

	// Update and check for death
	if (age >= lifetime)
	{
		// Recent death, so retire by moving alive count (and wrap)
		aliveIndex++;
		aliveIndex %= maxParticles;
		livingParticleCount--;
	}
}

void Emitter::CreateParticle()
{
	
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
	secondsPerParticle(1.0f / particlesPerSecond),
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
	if (paused) {
		return;
	}

	totalEmitterTime += deltaTime;
	timeSinceLastEmission += deltaTime;

	// Data to update?
	if (livingParticleCount <= 0) {
		return;
	}

	// Update new particles
	// Cyclic buffer?
	if (aliveIndex < deadIndex) {
		// All particles are contiguous
		// 0 -------- FIRST ALIVE ----------- FIRST DEAD -------- MAX
		// |    dead    |            alive       |         dead    |
		for (int i = aliveIndex; i < deadIndex; i++) {
			UpdateParticle(i);
		}
	}
	else if (deadIndex > aliveIndex) {
		// Alive particles wrap around
		// 0 -------- FIRST DEAD ----------- FIRST ALIVE -------- MAX
		// |    alive    |            dead       |         alive   |
		// Update first chunk
		for (int i = aliveIndex; i < maxParticles; i++) {
			UpdateParticle(i);
		}
		// Update second chunk
		for (int i = 0; i < deadIndex; i++) {
			UpdateParticle(i);
		}
	}
	else {
		// All particles are dead/alive
		for (int i = 0; i < maxParticles; i++) {
			UpdateParticle(i);
		}
	}

	// Emit Particles
	// Time to emit?
	while (timeSinceLastEmission > secondsPerParticle) {
		// Emit

		timeSinceLastEmission -= secondsPerParticle;
	}
}

void Emitter::Draw(std::shared_ptr<Camera> cam)
{
	
}

Emitter::~Emitter()
{
	delete[] particles;
}

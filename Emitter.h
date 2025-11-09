#pragma once
#include "Particle.h"
#include "Camera.h"
#include "Material.h"
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>

class Emitter
{
private:

	// Visual properties
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	float startSize;
	float endSize;
	bool constrainYAxis;
	bool paused;
	bool visible;

	// Movement
	DirectX::XMFLOAT3 startVelocity;
	DirectX::XMFLOAT3 acceleration;
	
	// Emission
	int maxParticles;
	int particlesPerSecond;
	float lifetime;
	float secondsPerParticle;
	float timeSinceLastEmission;
	float totalEmitterTime;

	// Sprite Sheet Options
	int spriteSheetWidth;
	int spriteSheetHeight;
	float spriteSheetFrameWidth;
	float spriteSheetFrameHeight;
	float spriteSheetSpeedScale;

	// Particle Info
	Particle* particles;
	int aliveIndex;
	int deadIndex;
	int livingParticleCount;

	// Randomization
	DirectX::XMFLOAT3 positionRandomRange;
	DirectX::XMFLOAT3 velocityRandomRange;
	DirectX::XMFLOAT2 rotationStartMinMax;
	DirectX::XMFLOAT2 rotationEndMinMax;

	// Rendering
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};

	// Mat and Transform
	std::shared_ptr<Material> material;
	std::shared_ptr<Transform> transform;

	void UpdateParticle(int particleIndex);
	void CreateParticle();

public:
	Emitter(
		int maxParticles,
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
		unsigned int spriteSheetWidth = 1,
		unsigned int spriteSheetHeight = 1,
		float spriteSheetSpeedScale = 1.0f,
		bool paused = false,
		bool visible = true
	);

	void Update(float deltaTime);
	void Draw(std::shared_ptr<Camera> cam);

	~Emitter();
};


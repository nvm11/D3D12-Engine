#pragma once
#include "Particle.h"
#include "Camera.h"
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>

class Emitter
{
private:
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;
	Particle* particles;
	int maxParticles;
	int particlesPerSecond;
	float maxParticleLifetime;
	float timeSinceLastEmission;
	DirectX::XMFLOAT3 emitterPos;
	DirectX::XMFLOAT3 positionRandomRange;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};

public:
	Emitter(const DirectX::XMFLOAT4 startColor,
			const DirectX::XMFLOAT4 endColor,
			const int maxParticles = 100, 
			const int particlesPerSecond,
			const float lifetime = 1.0f,
			const DirectX::XMFLOAT3 emitterPos = DirectX::XMFLOAT3(),
			const DirectX::XMFLOAT3 positionRandomRange = DirectX::XMFLOAT3());

	void Update(float deltaTime);
	void Draw(std::shared_ptr<Camera> cam);

	~Emitter();
};


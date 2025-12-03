#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Entity.h"
#include "Camera.h"
#include <vector>
#include "PathHelpers.h"
#include "Light.h"
#include "Emitter.h"

class Game
{
public:
	// Basic OOP setup
	Game();
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:
	const std::wstring assetPath = L"../../Assets/";
	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void CreateGeometry();
	void CreateRootSigAndPipelineState();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Pipeline
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	// Geometry
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};
	// Other graphics data
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	// Scene
	std::shared_ptr<Camera> camera;
	std::vector<std::shared_ptr<Entity>> entities;
	unsigned int lightCount = 0;
	std::vector<Light> lights;
	std::shared_ptr<Mesh> sphere;
	
	// Particle system
	std::vector<std::shared_ptr<Emitter>> emitters;

	// Refraction
	const float refractiveScale = 0.25f;
	Microsoft::WRL::ComPtr<ID3D12Resource> sceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneColorRTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneColorSRVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE sceneColorSRVHandle{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> refractionRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> refractionPipelineState;

	// Helper to test particle systems
	void InitializeParticleSystem();
	// Helper to set up RTVs for Refraction
	void SetupRefractionRTVs();
	// Helper to set up Root Sig and Pipeline State for Refraction
	void RefractionRootSigAndPipelineState();
};


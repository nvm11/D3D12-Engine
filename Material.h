#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
class Material
{
public:
	// --CONSTRUCTORS--
	Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
		DirectX::XMFLOAT3 colorTint = DirectX::XMFLOAT3(0, 0, 0),
		DirectX::XMFLOAT2 scale = DirectX::XMFLOAT2(0, 0),
		DirectX::XMFLOAT2 offset = DirectX::XMFLOAT2(0, 0));

	// --FUNCTIONS --

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

	// Getters
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState() const;
	DirectX::XMFLOAT2 GetUVScale() const;
	DirectX::XMFLOAT2 GetUVOffset() const;
	DirectX::XMFLOAT3 GetColorTint() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs() const;

	// Setters
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);
	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT3 colorTint);

private:
	// Material properties
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 scale = DirectX::XMFLOAT2(1, 1); // uv scale
	DirectX::XMFLOAT2 offset = DirectX::XMFLOAT2(0, 0); // uv offset

	// Pipeline state and descriptors
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	bool finalized;
	int highestSRVIndex = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[128]; // 128 textures can be bound per shader stage
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};


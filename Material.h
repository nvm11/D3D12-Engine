#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
class Material
{
public:
	// --FUNCTIONS --

	// Getters
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT3 GetColorTint();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs();

	// Setters
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);
	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT3 colorTint);
	
private:
	// Material properties
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 scale; // uv scale
	DirectX::XMFLOAT2 offset; // uv offset
	
	bool finalized;
	
	// Pipeline state and descriptors
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[128]; // 128 textures can be bound per shader stage
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};


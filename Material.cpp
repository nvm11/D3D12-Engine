#include "Material.h"
#include "Graphics.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
	DirectX::XMFLOAT3 colorTint,
	DirectX::XMFLOAT2 scale,
	DirectX::XMFLOAT2 offset)
	: pipelineState(pipelineState),
	colorTint(colorTint),
	scale(scale),
	offset(offset),
	finalized(false),
	highestSRVIndex(-1),
	metal(0.0f),
	roughness(0.0f),
	refractive(false)
{
	finalGPUHandleForSRVs = {};
	ZeroMemory(textureSRVsBySlot, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * 128);
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	// Check slot is valid
	if (slot < 0 || slot > 128) {
		return;
	}

	// Add to slot of handle
	textureSRVsBySlot[slot] = srv;
	highestSRVIndex = max(highestSRVIndex, slot);
}

void Material::FinalizeMaterial()
{
	// Check if already finalized
	if (finalized) {
		return;
	}

	// If not, copy SRVs to make them shader-visible
	for (int i = 0; i <= highestSRVIndex; i++) {
		// Copy the srv
		// (could do multiple if they were in the same heap)
		const auto gpuHandle = Graphics::CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);

		//Save the first handle for later reference
		if (i == 0) { finalGPUHandleForSRVs = gpuHandle;  }
	}

	// Mark material as complete
	finalized = true;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() const
{
	return pipelineState;
}

DirectX::XMFLOAT2 Material::GetUVScale() const
{
	return scale;
}

DirectX::XMFLOAT2 Material::GetUVOffset() const
{
	return offset;
}

DirectX::XMFLOAT3 Material::GetColorTint() const
{
	return colorTint;
}

float Material::GetRoughness() const
{
	return roughness;
}

float Material::GetMetal() const
{
	return metal;
}

bool Material::GetRefractive() const
{
	return refractive;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs() const
{
	return finalGPUHandleForSRVs;
}

void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	this->pipelineState = pipelineState;
}

void Material::SetUVScale(DirectX::XMFLOAT2 scale)
{
	this->scale = scale;
}

void Material::SetUVOffset(DirectX::XMFLOAT2 offset)
{
	this->offset = offset;
}

void Material::SetColorTint(DirectX::XMFLOAT3 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetRoughness(float roughness)
{
	this->roughness = roughness;
}

void Material::SetMetal(float metal)
{
	this->metal = metal;
}

void Material::SetRefractive(bool refractive)
{
	this->refractive = refractive;
}

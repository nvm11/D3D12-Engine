#include "Material.h"

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
    return pipelineState;
}

DirectX::XMFLOAT2 Material::GetUVScale()
{
    return scale;
}

DirectX::XMFLOAT2 Material::GetUVOffset()
{
    return offset;
}

DirectX::XMFLOAT3 Material::GetColorTint()
{
    return colorTint;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs()
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

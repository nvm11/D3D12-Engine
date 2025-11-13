#include "Emitter.h"
#include "Graphics.h"
#include "Vertex.h"
#include "PathHelpers.h"
// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// Macro for random float in range
#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min)

void Emitter::CreateParticles()
{
	// Delete and release existing resources
	if (particles) delete[] particles;
	if (indexBuffer)indexBuffer.Reset();

	// Set up the particle array
	particles = new Particle[maxParticles];
	ZeroMemory(particles, sizeof(Particle) * maxParticles);

	// Create an index buffer for particle drawing
	// indices as if we had two triangles per particle
	int numIndices = maxParticles * 6;
	unsigned int* indices = new unsigned int[numIndices];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}

	// Create the index buffer
	indexBuffer = Graphics::CreateStaticBuffer(sizeof(unsigned int), numIndices, indices);

	// Set up IB view
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = (UINT)(sizeof(unsigned int) * numIndices);
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

	// Clean up memory (already in gpu)
	delete[] indices;
}

void Emitter::InitializeGPUResources()
{
	UINT bufferSize = sizeof(Particle) * maxParticles;

	particleBuffer = Graphics::CreateBuffer(
		bufferSize,
		D3D12_HEAP_TYPE_UPLOAD,           // heapType
		D3D12_RESOURCE_STATE_GENERIC_READ // state
	);

	// Create Index Buffer
	CreateParticles(); // This creates the index buffer
}

void Emitter::CreateDescriptors()
{
	// Create SRV for particle structured buffer
	D3D12_SHADER_RESOURCE_VIEW_DESC particleSRVDesc = {};
	particleSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	particleSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	particleSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	particleSRVDesc.Buffer.NumElements = maxParticles;
	particleSRVDesc.Buffer.StructureByteStride = sizeof(Particle);
	particleSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// Use Graphics system to create the SRV in the global heap
	D3D12_CPU_DESCRIPTOR_HANDLE particleCPUHandle;
	Graphics::ReserveDescriptorHeapSlot(&particleCPUHandle, &particleBufferSRVHandle);
	Graphics::Device->CreateShaderResourceView(particleBuffer.Get(), &particleSRVDesc, particleCPUHandle);
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

void Emitter::EmitParticle()
{
	// Can we emit?
	if (livingParticleCount == maxParticles) {
		return;
	}

	particles[deadIndex].EmitTime = totalEmitterTime;

	// Update first dead particle
	particles[deadIndex].StartPosition = transform->GetPosition();
	particles[deadIndex].StartPosition.x += positionRandomRange.x * RandomRange(-1.0f, 1.0f);
	particles[deadIndex].StartPosition.y += positionRandomRange.y * RandomRange(-1.0f, 1.0f);
	particles[deadIndex].StartPosition.z += positionRandomRange.z * RandomRange(-1.0f, 1.0f);

	// Adjust particle start velocity based on random range
	particles[deadIndex].StartVelocity = startVelocity;
	particles[deadIndex].StartVelocity.x += velocityRandomRange.x * RandomRange(-1.0f, 1.0f);
	particles[deadIndex].StartVelocity.y += velocityRandomRange.y * RandomRange(-1.0f, 1.0f);
	particles[deadIndex].StartVelocity.z += velocityRandomRange.z * RandomRange(-1.0f, 1.0f);
	
	// Adjust start and end rotation values based on range
	particles[deadIndex].StartRotation = RandomRange(rotationStartMinMax.x, rotationStartMinMax.y);
	particles[deadIndex].EndRotation = RandomRange(rotationEndMinMax.x, rotationEndMinMax.y);

	// Increment dead particle
	deadIndex++;
	deadIndex %= maxParticles; // wrap

	livingParticleCount++;
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
	transform = std::make_shared<Transform>();
	transform->SetPosition(emitterPosition);

	// Set up emission stats
	timeSinceLastEmission = 0;
	livingParticleCount = 0;
	aliveIndex = 0;
	deadIndex = 0;

	// Create related structured buffer
	InitializeGPUResources();
	// Create the pipeline
	CreateRootSigAndPipelineState();
	// Create descriptors
	CreateDescriptors();
}

void Emitter::CreateRootSigAndPipelineState()
{
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load both shaders
	D3DReadFileToBlob(FixPath(L"ParticlesVS.cso").c_str(), vertexShaderByteCode.GetAddressOf());
	D3DReadFileToBlob(FixPath(L"ParticlesPS.cso").c_str(), pixelShaderByteCode.GetAddressOf());

	// Root parameters: CBV (vert and pixel), SRV for particle buffer, SRV for texture
	D3D12_ROOT_PARAMETER rootParams[4] = {};

	// CBV for vertex shader (register b0)
	D3D12_DESCRIPTOR_RANGE cbvVertex = {};
	cbvVertex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cbvVertex.NumDescriptors = 1;
	cbvVertex.BaseShaderRegister = 0;
	cbvVertex.RegisterSpace = 0;
	cbvVertex.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &cbvVertex;

	// CBV for pixel shader (register b1)
	D3D12_DESCRIPTOR_RANGE cbvPixel = {};
	cbvPixel.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cbvPixel.NumDescriptors = 1;
	cbvPixel.BaseShaderRegister = 0;
	cbvPixel.RegisterSpace = 0;
	cbvPixel.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &cbvPixel;

	// SRV for particle structured buffer (register t0)
	D3D12_DESCRIPTOR_RANGE srvRange1 = {};
	srvRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange1.NumDescriptors = 1;
	srvRange1.BaseShaderRegister = 0;
	srvRange1.RegisterSpace = 0;
	srvRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange1;

	// SRV for particle texture (register t1)
	D3D12_DESCRIPTOR_RANGE srvRange2 = {};
	srvRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange2.NumDescriptors = 1;
	srvRange2.BaseShaderRegister = 0;
	srvRange2.RegisterSpace = 0;
	srvRange2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[3].DescriptorTable.pDescriptorRanges = &srvRange2;

	// Static sampler for texture (register s0)
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxAnisotropy = 0;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 4;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &sampler;

	ID3DBlob* serializedRootSig = 0;
	ID3DBlob* errors = 0;

	D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&errors);

	// Check for errors during serialization
	if (errors != 0)
	{
		OutputDebugString((wchar_t*)errors->GetBufferPointer());
	}

	// Actually create the root sig
	Graphics::Device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.GetAddressOf()));


	// Pipeline State
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
	psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
	psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX; // Enable all samples

	psoDesc.InputLayout.NumElements = 0;
	psoDesc.InputLayout.pInputElementDescs = nullptr;

	// Enable alpha blending for particles
	psoDesc.BlendState.RenderTarget[0].BlendEnable = false;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Rasterizer state
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FrontCounterClockwise = false;
	psoDesc.RasterizerState.DepthBias = 0;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	psoDesc.RasterizerState.DepthClipEnable = true;
	psoDesc.RasterizerState.MultisampleEnable = false;
	psoDesc.RasterizerState.AntialiasedLineEnable = false;
	psoDesc.RasterizerState.ForcedSampleCount = 0;

	// Depth settings - often disable depth writing for particles
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Don't write depth
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	// Create the pipe state object
	Graphics::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
	material->SetPipelineState(pipelineState);
}

void Emitter::Update(float deltaTime)
{
	// Debug output
	static float debugTimer = 0;
	debugTimer += deltaTime;
	if (debugTimer > 1.0f) {
		printf("Particles: %d alive, %d total\n", livingParticleCount, maxParticles);
		debugTimer = 0;
	}

	if (paused) {
		return;
	}

	totalEmitterTime += deltaTime;
	timeSinceLastEmission += deltaTime;

	
	// Update new particles
	// Cyclic buffer
	if (livingParticleCount > 0)
	{
		if (aliveIndex < deadIndex) {
			// All particles are contiguous
			// 0 -------- FIRST ALIVE ----------- FIRST DEAD -------- MAX
			// |    dead    |            alive       |         dead    |
			for (int i = aliveIndex; i < deadIndex; i++) {
				UpdateParticle(i);
			}
		}
		else if (deadIndex < aliveIndex) {
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
	}

	// Emit Particles
	// Time to emit?
	while (timeSinceLastEmission > secondsPerParticle) {
		// Emit
		EmitParticle();
		timeSinceLastEmission -= secondsPerParticle;
	}
}

void Emitter::Draw(std::shared_ptr<Camera> cam)
{
	// Need to emit?
	if (!visible || livingParticleCount <= 0) {
		return;
	}
	

	// Update particle buffer data
	D3D12_RANGE readRange = { 0, 0 };
	void* mappedData;
	particleBuffer->Map(0, &readRange, &mappedData);
	memcpy(mappedData, particles, sizeof(Particle) * maxParticles);
	particleBuffer->Unmap(0, nullptr);

	// Set pipeline state
	Graphics::CommandList->SetPipelineState(pipelineState.Get());
	Graphics::CommandList->SetGraphicsRootSignature(rootSignature.Get());

	// Set topology and index buffer
	Graphics::CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Graphics::CommandList->IASetIndexBuffer(&ibView);

	// Create separate constant buffers
	// Update cbuffer data
	ParticleExternalData particleData = {};

	particleData.view = cam->GetView();
	particleData.projection = cam->GetProjection();
	particleData.startColor = startColor;
	particleData.endColor = endColor;
	particleData.currentTime = totalEmitterTime;
	particleData.acceleration = acceleration;
	particleData.spriteSheetWidth = spriteSheetWidth;
	particleData.spriteSheetHeight = spriteSheetHeight;
	particleData.spriteSheetFrameWidth = spriteSheetFrameWidth;
	particleData.spriteSheetFrameHeight = spriteSheetFrameHeight;
	particleData.spriteSheetSpeedScale = spriteSheetSpeedScale;
	particleData.startSize = startSize;
	particleData.endSize = endSize;
	particleData.lifetime = lifetime;
	particleData.constrainYAxis = constrainYAxis ? 1 : 0;
	if (material) {
		particleData.colorTint = material->GetColorTint();
	}
	else {
		particleData.colorTint = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); // Default white
	}

	DirectX::XMFLOAT3 pixelData = { material->GetColorTint() };

	D3D12_GPU_DESCRIPTOR_HANDLE vertexCBVHandle = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
		(void*)(&particleData), sizeof(ParticleExternalData));

	D3D12_GPU_DESCRIPTOR_HANDLE pixelCBVHandle = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
		(void*)(&pixelData), sizeof(DirectX::XMFLOAT3));

	// Set them separately
	Graphics::CommandList->SetGraphicsRootDescriptorTable(0, vertexCBVHandle);
	Graphics::CommandList->SetGraphicsRootDescriptorTable(1, pixelCBVHandle);

	// Parameter 2: SRV for particle buffer (t0)
	Graphics::CommandList->SetGraphicsRootDescriptorTable(2, particleBufferSRVHandle);

	// Parameter 3: SRV for texture (t1)
	Graphics::CommandList->SetGraphicsRootDescriptorTable(3, material->GetFinalGPUHandleForSRVs());
	// Draw
	Graphics::CommandList->DrawIndexedInstanced(livingParticleCount * 6, 1, 0, 0, 0);
}

Emitter::~Emitter()
{
	delete[] particles;
}

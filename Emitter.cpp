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
	indexBuffer.Reset();
	particleBuffer.Reset();

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
	indexBuffer = Graphics::CreateStaticBuffer(sizeof(unsigned int), indexCount, indices);

	// Set up IB view
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = (UINT)(sizeof(unsigned int) * indexCount);
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

	// Clean up memory (already in gpu)
	delete[] indices;
}

void Emitter::InitializeGPUResources()
{
	// Create Structured Buffer for particles (your existing code)
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

	// Create Constant Buffer
	UINT constantBufferSize = sizeof(ParticleExternalData);
	constantBufferSize = (constantBufferSize + 255) & ~255; // Align to 256 bytes

	D3D12_HEAP_PROPERTIES cbHeapProps = {};
	cbHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC cbDesc = {};
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Width = constantBufferSize;
	cbDesc.Height = 1;
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1;
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Graphics::Device->CreateCommittedResource(
		&cbHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBuffer)
	);

	// Map constant buffer
	constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constantBufferData));

	// Create Index Buffer
	CreateParticles(); // This creates the index buffer
}

void Emitter::CreateConstantBuffer(std::shared_ptr<Camera> cam)
{
	// Set constant buffer data
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
	particleData.colorTint = material->GetColorTint();

	// Send data to the buffer
	memcpy(constantBufferData, &particleData, sizeof(ParticleExternalData));
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
	if (livingParticleCount >= maxParticles) {
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

	// Create the pipeline
	CreateRootSigAndPipelineState();
	// Create related structured buffer
	InitializeGPUResources();
	// Create descriptors
	CreateDescriptors();
}

void Emitter::CreateRootSigAndPipelineState()
{
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load both shaders
	D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
	D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());

	// No need for input layout since no use for vertex shader
	//// Input Layout
	//const unsigned int inputElementCount = 3;
	//D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	//{
	//	// Create an input layout that describes the vertex format
	//	// used by the vertex shader we're using
	//	//  - This is used by the pipeline to know how to interpret the raw data
	//	//     sitting inside a vertex buffer

	//	// Set up the first element - a position, which is 3 float values
	//	inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // How far into the vertex is this?  Assume it's after the previous element
	//	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// Most formats are described as color channels, really it just means "Three 32-bit floats"
	//	inputElements[0].SemanticName = "POSITION";					// This is "POSITTION" - needs to match the semantics in our vertex shader input!
	//	inputElements[0].SemanticIndex = 0;							// This is the 0th position (there could be more)

	//	// Set up the second element - a UV, which is 2 more float values
	//	inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;	// After the previous element
	//	inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;			// 2x 32-bit floats
	//	inputElements[1].SemanticName = "TEXCOORD";					// Match our vertex shader input!
	//	inputElements[1].SemanticIndex = 0;							// This is the 0th uv (there could be more)

	//	// Set up the fourth element - a tangent, which is 2 more float values
	//	inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;	// After the previous element
	//	inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// 3x 32-bit floats
	//	inputElements[3].SemanticName = "TANGENT";					// Match our vertex shader input!
	//	inputElements[3].SemanticIndex = 0;							// This is the 0th tangent (there could be more)
	//}
	// Root parameters: CBV, SRV for particle buffer, SRV for texture
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	// CBV for particle constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].Descriptor.ShaderRegister = 0;

	// SRV for particle structured buffer
	D3D12_DESCRIPTOR_RANGE srvRange1 = {};
	srvRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange1.NumDescriptors = 1;
	srvRange1.BaseShaderRegister = 0;
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange1;

	// SRV for particle texture
	D3D12_DESCRIPTOR_RANGE srvRange2 = {};
	srvRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange2.NumDescriptors = 1;
	srvRange2.BaseShaderRegister = 1;
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange2;

	// Static sampler for texture
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 3;
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

	psoDesc.InputLayout.NumElements = 0;
	psoDesc.InputLayout.pInputElementDescs = nullptr; 

	// Enable alpha blending for particles
	psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Depth settings - often disable depth writing for particles
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Don't write depth
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// Create the pipe state object
	Graphics::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
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
		EmitParticle();
		timeSinceLastEmission -= secondsPerParticle;
	}
}

void Emitter::Draw(std::shared_ptr<Camera> cam)
{
	// Need to emit?
	if (!visible && livingParticleCount <= 0) {
		return;
	}

	// Update constant buffer data
	CreateConstantBuffer(cam);

	// Update particle buffer data
	D3D12_RANGE readRange = { 0, 0 };
	void* mappedData;
	particleBuffer->Map(0, &readRange, &mappedData);
	memcpy(mappedData, particles, sizeof(Particle) * maxParticles);
	particleBuffer->Unmap(0, nullptr);

	// Set pipeline state
	Graphics::CommandList->SetPipelineState(pipelineState.Get());
	Graphics::CommandList->SetGraphicsRootSignature(rootSignature.Get());


	// Set vertex buffer
	Graphics::CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Graphics::CommandList->IASetIndexBuffer(&ibView);

	// Set CBV (root parameter 0)
	Graphics::CommandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());

	// Set SRV for particle buffer (root parameter 1)
	Graphics::CommandList->SetGraphicsRootDescriptorTable(1, particleBufferSRVHandle);

	// Set SRV for texture from material (root parameter 2)
	Graphics::CommandList->SetGraphicsRootDescriptorTable(2, material->GetFinalGPUHandleForSRVs());

	// Draw
	Graphics::CommandList->DrawIndexedInstanced(livingParticleCount * 6, 1, 0, 0, 0);
}

Emitter::~Emitter()
{
	delete[] particles;
}

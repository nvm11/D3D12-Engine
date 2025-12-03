#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "BufferStructs.h"
#include "RayTracing.h"
#include <iostream>

#include <DirectXMath.h>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game()
{
	CreateRootSigAndPipelineState();
	SetupRefractionRTVs();
	CreateGeometry();
	RefractionRootSigAndPipelineState();

	camera = std::make_shared<Camera>(XMFLOAT3(0.0f, 0.0f, 0.0f),
		XM_PIDIV4,
		Window::AspectRatio(),
		0.01f,
		100.0f);

	// Create Emitter
	//InitializeParticleSystem();

	// Initialize raytracing
	//RayTracing::Initialize(
	//	Window::Width(),
	//	Window::Height(),
	//	FixPath(L"RayTracing.cso"));

	// Create a BLAS for a single mesh, then the TLAS for our “scene”
	//RayTracing::CreateTopLevelAccelerationStructureForScene(entities);
	// Finalize any initialization and wait for the GPU
	// before proceeding to the game loop

	Graphics::CloseAndExecuteCommandList();
	Graphics::WaitForGPU();
	Graphics::ResetAllocatorAndCommandList(0);
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// Wait for GPU before shutdown
	Graphics::WaitForGPU();
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Load Textures
	// Cobblestone
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneAlbedo = Graphics::LoadTexture(FixPath(assetPath + L"Textures/cobblestone_albedo.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneNormals = Graphics::LoadTexture(FixPath(assetPath + L"Textures/cobblestone_normals.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneRoughness = Graphics::LoadTexture(FixPath(assetPath + L"Textures/cobblestone_roughness.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneMetal = Graphics::LoadTexture(FixPath(assetPath + L"Textures/cobblestone_metal.png").c_str());

	//Create materials
	// Samplers are handled by a single static sampler
	// This can be found in the root signature
	// Cobblestone
	std::shared_ptr<Material> cobbleMat = std::make_shared<Material>(pipelineState);
	cobbleMat->AddTexture(cobblestoneAlbedo, 0);
	cobbleMat->AddTexture(cobblestoneNormals, 1);
	cobbleMat->AddTexture(cobblestoneRoughness, 2);
	cobbleMat->AddTexture(cobblestoneMetal, 3);
	cobbleMat->FinalizeMaterial();

	std::shared_ptr<Material> refractiveMat = std::make_shared<Material>(refractionPipelineState);
	refractiveMat->AddTexture(cobblestoneNormals, 0);
	refractiveMat->AddTexture(sceneColorSRVHandle, 1);
	refractiveMat->SetRefractive(true);
	refractiveMat->FinalizeMaterial();

	// Load meshes
	const std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(FixPath(assetPath + L"Meshes/cube.obj").c_str());
	const std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(FixPath(assetPath + L"Meshes/sphere.obj").c_str());
	const std::shared_ptr<Mesh> helix = std::make_shared<Mesh>(FixPath(assetPath + L"Meshes/helix.obj").c_str());
	const std::shared_ptr<Mesh> torus = std::make_shared<Mesh>(FixPath(assetPath + L"Meshes/torus.obj").c_str());
	const std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(FixPath(assetPath + L"Meshes/cylinder.obj").c_str());

	// Create entities
	std::shared_ptr<Entity> entityCube = std::make_shared<Entity>(cube);
	entityCube->GetTransform()->SetPosition(5, 0, 0);

	std::shared_ptr<Entity> entityHelix = std::make_shared<Entity>(helix);
	entityHelix->GetTransform()->SetPosition(5, 5, 0);

	std::shared_ptr<Entity> entitySphere = std::make_shared<Entity>(sphere);
	entitySphere->GetTransform()->SetPosition(-5, 0, 0);

	std::shared_ptr<Entity> entityTorus = std::make_shared<Entity>(torus);
	entitySphere->GetTransform()->SetPosition(-0, 5, 0);

	// Add to list
	// Cube will be refractive
	//entities.push_back(entityCube);
	entities.push_back(entityHelix);
	entities.push_back(entitySphere);
	entities.push_back(entityTorus);

	// Add material to entites
	for (auto& e : entities) {
		e->SetMaterial(cobbleMat);
	}

	// Add refractive entities
	entityCube->SetMaterial(refractiveMat);
	entities.push_back(entityCube);

	// Create Lights
	Light directionLight = {};
	directionLight.type = LIGHT_TYPE_DIRECTIONAL;
	directionLight.direction = XMFLOAT3(-1.0f, 0.0f, 1.0f);
	directionLight.color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionLight.intensity = 1.0f;

	// Add to vector
	lights.push_back(directionLight);
	// Increment light count
	lightCount++;
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		// Create an input layout that describes the vertex format
		// used by the vertex shader we're using
		//  - This is used by the pipeline to know how to interpret the raw data
		//     sitting inside a vertex buffer

		// Set up the first element - a position, which is 3 float values
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // How far into the vertex is this?  Assume it's after the previous element
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// Most formats are described as color channels, really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";					// This is "POSITTION" - needs to match the semantics in our vertex shader input!
		inputElements[0].SemanticIndex = 0;							// This is the 0th position (there could be more)

		// Set up the second element - a UV, which is 2 more float values
		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;	// After the previous element
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;			// 2x 32-bit floats
		inputElements[1].SemanticName = "TEXCOORD";					// Match our vertex shader input!
		inputElements[1].SemanticIndex = 0;							// This is the 0th uv (there could be more)

		// Set up the third element - a normal, which is 3 more float values
		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;	// After the previous element
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// 3x 32-bit floats
		inputElements[2].SemanticName = "NORMAL";					// Match our vertex shader input!
		inputElements[2].SemanticIndex = 0;							// This is the 0th normal (there could be more)

		// Set up the fourth element - a tangent, which is 2 more float values
		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;	// After the previous element
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// 3x 32-bit floats
		inputElements[3].SemanticName = "TANGENT";					// Match our vertex shader input!
		inputElements[3].SemanticIndex = 0;							// This is the 0th tangent (there could be more)
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4;		// Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0;	// Starts at t0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		// Note: This is in lieu of having materials have their own samplers for this demo
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0;  // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
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
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// Overall primitive topology type (triangle, line, etc.) is set here 
		// IASetPrimTop() is still used to set list/strip/adj options
		// See: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/managing-graphics-pipeline-state-in-direct3d-12

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		Graphics::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}

	// Set up the viewport and scissor rectangle
	{
		// Set up the viewport so we render into the correct
		// portion of the render target
		viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)Window::Width();
		viewport.Height = (float)Window::Height();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// Define a scissor rectangle that defines a portion of
		// the render target for clipping.  This is different from
		// a viewport in that it is applied after the pixel shader.
		// We need at least one of these, but we're rendering to 
		// the entire window, so it'll be the same size.
		scissorRect = {};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = Window::Width();
		scissorRect.bottom = Window::Height();
	}
}

void Game::InitializeParticleSystem()
{
	// Load a texture for particles (create a simple particle texture first)
	D3D12_CPU_DESCRIPTOR_HANDLE explosionTexture = Graphics::LoadTexture(FixPath(assetPath + L"Particles/explosion_spritesheet.png").c_str());

	// Create explosion material
	std::shared_ptr<Material> explosionMat = std::make_shared<Material>(pipelineState);
	explosionMat->AddTexture(explosionTexture, 0);
	explosionMat->FinalizeMaterial();

	D3D12_CPU_DESCRIPTOR_HANDLE magicTexture = Graphics::LoadTexture(FixPath(assetPath + L"Particles/magic_05.png").c_str());

	std::shared_ptr<Material> magicMat = std::make_shared<Material>(pipelineState);
	magicMat->AddTexture(magicTexture, 0);
	magicMat->FinalizeMaterial();

	D3D12_CPU_DESCRIPTOR_HANDLE sparkTexture = Graphics::LoadTexture(FixPath(assetPath + L"Particles/slash_03.png").c_str());

	std::shared_ptr<Material> sparkMat = std::make_shared<Material>(pipelineState);
	sparkMat->AddTexture(sparkTexture, 0);
	sparkMat->FinalizeMaterial();

	// Create particle emitter with various parameters
	emitters.push_back(std::make_shared<Emitter>(
		1000,                          // maxParticles
		100,                            // particlesPerSecond  
		1.0f,                          // lifetime
		1.0f,                          // startSize
		1.0f,                         // endSize
		false,                         // constrainYAxis
		XMFLOAT4(1.0f, 1.0f, 0.1f, 1.0f), // startColor (orange)
		XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), // endColor (red, fading out)
		XMFLOAT3(0.0f, 5.0f, 0.0f),    // startVelocity
		XMFLOAT3(1.0f, 0.5f, 1.0f),    // velocityRandomRange
		XMFLOAT3(-1.0f, 0.0f, 3.0f),   // emitterPosition
		XMFLOAT3(0.25f, 0.0f, 0.25f),    // positionRandomRange
		XMFLOAT2(0.0f, XM_2PI),        // rotationStartMinMax
		XMFLOAT2(0.0f, XM_2PI),        // rotationEndMinMax
		XMFLOAT3(0.0f, 0.5f, 0.0f),   // acceleration
		explosionMat,                   // material
		5,                             // spriteSheetWidth
		5,                             // spriteSheetHeight
		1.0f,                          // spriteSheetSpeedScale
		false,                         // paused
		true                           // visible
	));

	emitters.push_back(std::make_shared<Emitter>(
		100,
		10,
		0.5f,
		0.05f,
		1.0f,
		false,
		XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f),
		XMFLOAT4(0.0f, 1.0f, 1.0f, 0.0f),
		XMFLOAT3(0.5f, -0.5f, 1.0f),
		XMFLOAT3(1.0f, -1.0f, 1.0f),
		XMFLOAT3(1.0f, 0.0f, 1.0f),
		XMFLOAT3(0.5f, 0.0f, 0.5f),
		XMFLOAT2(0.0f, XM_PI),
		XMFLOAT2(0.0f, XM_PI),
		XMFLOAT3(0.0f, -0.5f, 1.0f),
		magicMat,
		1,
		1,
		1.0f,
		false,
		true
	));

	emitters.push_back(std::make_shared<Emitter>(
		2000,                          // maxParticles
		400,                            // particlesPerSecond  
		3.0f,                          // lifetime
		0.1f,                          // startSize
		0.5f,                         // endSize
		false,                         // constrainYAxis
		XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), // startColor (orange)
		XMFLOAT4(0.0f, 1.0f, 0.5f, 1.0f), // endColor (red, fading out)
		XMFLOAT3(1.0f, 0.4f, 0.0f),    // startVelocity
		XMFLOAT3(1.0f, 0.5f, 0.0f),    // velocityRandomRange
		XMFLOAT3(4.0f, 0.0f, 3.0f),   // emitterPosition
		XMFLOAT3(0.2f, 0.2f, 0.2f),    // positionRandomRange
		XMFLOAT2(0.0f, 0.0f),        // rotationStartMinMax
		XMFLOAT2(0.0f, 0.0f),        // rotationEndMinMax
		XMFLOAT3(0.0f, 0.8f, 0.0f),   // acceleration
		sparkMat,                   // material
		5,                             // spriteSheetWidth
		5,                             // spriteSheetHeight
		1.0f,                          // spriteSheetSpeedScale
		false,                         // paused
		true                           // visible
	));
}

void Game::SetupRefractionRTVs()
{
	// Describe Render Target
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = Window::Width();
	desc.Height = Window::Height();
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// Setup Clear Value
	D3D12_CLEAR_VALUE clear = {};
	clear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clear.Color[0] = 0.2f; // R
	clear.Color[1] = 0.2f; // G  
	clear.Color[2] = 0.45f; // B
	clear.Color[3] = 1.0f; // A

	// Describe Memory Heap
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // GPU-only memory
	heapProps.VisibleNodeMask = 1;

	// Create the Resource
	Graphics::Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear,
		IID_PPV_ARGS(sceneColorRTV.GetAddressOf()));

	// Create RTV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	Graphics::Device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(sceneColorRTVHeap.GetAddressOf()));

	// Get CPU handle
	sceneColorRTVHandle = sceneColorRTVHeap->GetCPUDescriptorHandleForHeapStart();
	// Create actual RTV
	Graphics::Device->CreateRenderTargetView(
		sceneColorRTV.Get(),
		0,
		sceneColorRTVHandle);


	// Create SRV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	srvHeapDesc.NodeMask = 0;

	Graphics::Device->CreateDescriptorHeap(
		&srvHeapDesc,
		IID_PPV_ARGS(sceneColorSRVHeap.GetAddressOf()));

	// Create corresponding SRV Handle
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	// Get CPU handle
	sceneColorSRVHandle = sceneColorSRVHeap->GetCPUDescriptorHandleForHeapStart();
	// Create SRV
	Graphics::Device->CreateShaderResourceView(sceneColorRTV.Get(), &srvDesc, sceneColorSRVHandle);
}

void Game::RefractionRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> refractionShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"RefractionPS.cso").c_str(), refractionShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		// Create an input layout that describes the vertex format
		// used by the vertex shader we're using
		//  - This is used by the pipeline to know how to interpret the raw data
		//     sitting inside a vertex buffer

		// Set up the first element - a position, which is 3 float values
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // How far into the vertex is this?  Assume it's after the previous element
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;		// Most formats are described as color channels, really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";					// This is "POSITTION" - needs to match the semantics in our vertex shader input!
		inputElements[0].SemanticIndex = 0;							// This is the 0th position (there could be more)

		// Set up the second element - a UV, which is 2 more float values
		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // After the previous element
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0;

		// Set up the third element - a normal, which is 3 more float values
		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0;

		// Set up the fourth element - a tangent, which is 2 more float values
		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0;
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 3;		// Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0;	// Starts at t0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single sampler
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0;  // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Create a Clamp Sampler
		D3D12_STATIC_SAMPLER_DESC clampSamp = {};
		clampSamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamp.Filter = D3D12_FILTER_ANISOTROPIC;
		clampSamp.MaxAnisotropy = 16;
		clampSamp.MaxLOD = D3D12_FLOAT32_MAX;
		clampSamp.ShaderRegister = 1;  // register(s0)
		clampSamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap, clampSamp };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
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
			IID_PPV_ARGS(refractionRootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// Overall primitive topology type (triangle, line, etc.) is set here 
		// IASetPrimTop() is still used to set list/strip/adj options
		// See: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/managing-graphics-pipeline-state-in-direct3d-12

		// Root sig
		psoDesc.pRootSignature = refractionRootSignature.Get();

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = refractionShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = refractionShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		Graphics::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(refractionPipelineState.GetAddressOf()));
	}
}



// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Resize the viewport and scissor rectangle
	{
		// Set up the viewport so we render into the correct
		// portion of the render target
		viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)Window::Width();
		viewport.Height = (float)Window::Height();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// Define a scissor rectangle that defines a portion of
		// the render target for clipping.  This is different from
		// a viewport in that it is applied after the pixel shader.
		// We need at least one of these, but we're rendering to 
		// the entire window, so it'll be the same size.
		scissorRect = {};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = Window::Width();
		scissorRect.bottom = Window::Height();
	}

	if (camera) {
		camera->UpdateProjectionMatrix(Window::AspectRatio());
	}

	sceneColorRTV.Reset();
	sceneColorRTVHeap.Reset();
	SetupRefractionRTVs();

	// Resize raytracing output texture
	//RayTracing::ResizeOutputUAV(Window::Width(), Window::Height());
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();

	camera->Update(deltaTime);

	//for (auto& e : emitters) {
	//	e->Update(deltaTime);
	//}
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Transition Scene RTV to Render Target
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = sceneColorRTV.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // From previous frame
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Graphics::CommandList->ResourceBarrier(1, &barrier);
	}

	// Render Opaque objects to Scene RTV
	float clearColor[] = { 0.2f, 0.2f, 0.45f, 1.0f };
	{

		Graphics::CommandList->ClearRenderTargetView(
			sceneColorRTVHandle,
			clearColor,
			0,
			0);
		Graphics::CommandList->ClearDepthStencilView(
			Graphics::DSVHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,	// Max Depth value
			0,		// Not Clearing Stencil, but needs values
			0, 0);   // No scissor rects

		// Set Overall pipeline state
		Graphics::CommandList->SetPipelineState(pipelineState.Get());
		// Root Sig
		Graphics::CommandList->SetGraphicsRootSignature(rootSignature.Get());
		// Descriptor Heap
		Graphics::CommandList->SetDescriptorHeaps(1, Graphics::CBVSRVDescriptorHeap.GetAddressOf());

		// Set Render target and viewport
		Graphics::CommandList->OMSetRenderTargets(1, &sceneColorRTVHandle, true, &Graphics::DSVHandle);
		Graphics::CommandList->RSSetViewports(1, &viewport);
		Graphics::CommandList->RSSetScissorRects(1, &scissorRect);
		Graphics::CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Render Opaque Entities
		for (auto& e : entities) {
			if (e->GetMaterial()->GetRefractive()) {
				continue;
			}

			std::shared_ptr<Material> mat = e->GetMaterial();

			{
				// Set Pipeline State
				Graphics::CommandList->SetPipelineState(mat->GetPipelineState().Get());

				// Set the SRV Descriptor Handle (assumes descriptor table 2 is for textures)
				Graphics::CommandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForSRVs());
			}

			// Set up the data we intend to use for drawing this entity
			{
				VertexShaderExternalData vsData = {};
				vsData.world = e->GetTransform()->GetWorldMatrix();
				vsData.worldInverseTranspose = e->GetTransform()->GetWorldInverseTransposeMatrix();
				vsData.view = camera->GetView();
				vsData.projection = camera->GetProjection();

				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
				D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
					(void*)(&vsData), sizeof(VertexShaderExternalData));

				// Set this constant buffer handle
				Graphics::CommandList->SetGraphicsRootDescriptorTable(0, cbHandle);
			}

			// Pixel shader data and cbuffer setup
			{
				PixelShaderExternalData psData = {};
				psData.uvScale = mat->GetUVScale();
				psData.uvOffset = mat->GetUVOffset();
				psData.cameraPosition = camera->GetTransform().GetPosition();
				psData.lightCount = lightCount;
				memcpy(psData.lights, &lights[0], sizeof(Light) * MAX_LIGHTS);

				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
				D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
					(void*)(&psData), sizeof(PixelShaderExternalData));

				// Set this constant buffer handle
				// Note: This assumes that descriptor table 1 is the
				//       place to put this particular descriptor.  This
				//       is based on how we set up our root signature.
				Graphics::CommandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
			}

			// Grab the mesh and its buffer views
			std::shared_ptr<Mesh> mesh = e->GetMesh();
			D3D12_VERTEX_BUFFER_VIEW vbv = mesh->GetVertexBufferView();
			D3D12_INDEX_BUFFER_VIEW  ibv = mesh->GetIndexBufferView();

			// Set the geometry
			Graphics::CommandList->IASetVertexBuffers(0, 1, &vbv);
			Graphics::CommandList->IASetIndexBuffer(&ibv);

			// Draw
			Graphics::CommandList->DrawIndexedInstanced((UINT)mesh->GetIndexCount(), 1, 0, 0, 0);
		}
	}

	// Transition Scene Color RTV to Resource State
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = sceneColorRTV.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Graphics::CommandList->ResourceBarrier(1, &barrier);
	}

	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = Graphics::BackBuffers[Graphics::SwapChainIndex()];

	{
		// Transition back buffer to copy dest
		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = currentBackBuffer.Get();
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = sceneColorRTV.Get();
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		Graphics::CommandList->ResourceBarrier(2, barriers);

		// Copy opaque RTV to back buffer
		Graphics::CommandList->CopyResource(currentBackBuffer.Get(), sceneColorRTV.Get());

		// Transition back to appropriate states
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		Graphics::CommandList->ResourceBarrier(2, barriers);
	}

	// Set Back Buffer as Render Target
	{
		// Set Overall pipeline state
		Graphics::CommandList->SetPipelineState(refractionPipelineState.Get());
		// Root Sig
		Graphics::CommandList->SetGraphicsRootSignature(refractionRootSignature.Get());

		Graphics::CommandList->OMSetRenderTargets(1, &Graphics::RTVHandles[Graphics::SwapChainIndex()], true, &Graphics::DSVHandle);
		Graphics::CommandList->RSSetViewports(1, &viewport);
		Graphics::CommandList->RSSetScissorRects(1, &scissorRect);
	}

	// Render Refractive Entities
	for (auto& e : entities) {
		if (!e->GetMaterial()->GetRefractive()) {
			continue;
		}

		std::shared_ptr<Material> mat = e->GetMaterial();

		Graphics::CommandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForSRVs());

		// Set up the data we intend to use for drawing this entity
		{
			VertexShaderExternalData vsData = {};
			vsData.world = e->GetTransform()->GetWorldMatrix();
			vsData.worldInverseTranspose = e->GetTransform()->GetWorldInverseTransposeMatrix();
			vsData.view = camera->GetView();
			vsData.projection = camera->GetProjection();

			// Send this to a chunk of the constant buffer heap
			// and grab the GPU handle for it so we can set it for this draw
			D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
				(void*)(&vsData), sizeof(VertexShaderExternalData));

			// Set this constant buffer handle
			Graphics::CommandList->SetGraphicsRootDescriptorTable(0, cbHandle);
		}

		// Set Up Data used for Pixel Shader
		{
			RefractiveExternalData psData = {};
			psData.lightCount = lightCount;
			psData.clearColor = DirectX::XMFLOAT3(clearColor);
			psData.uvScale = mat->GetUVScale();
			psData.uvOffset = mat->GetUVOffset();
			psData.screenWidth = Window::Width();
			psData.screenHeight = Window::Height();
			psData.refractionScale = refractiveScale;
			psData.useRefractionSilhouette = false;
			memcpy(psData.lights, &lights[0], sizeof(Light)* MAX_LIGHTS);

			// Send this to a chunk of the constant buffer heap
			// and grab the GPU handle for it so we can set it for this draw
			D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS = Graphics::FillNextConstantBufferAndGetGPUDescriptorHandle(
				(void*)(&psData), sizeof(PixelShaderExternalData));

			// Set this constant buffer handle
			// Note: This assumes that descriptor table 1 is the
			//       place to put this particular descriptor.  This
			//       is based on how we set up our root signature.
			Graphics::CommandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
		}

		// Grab the mesh and its buffer views
		std::shared_ptr<Mesh> mesh = e->GetMesh();
		D3D12_VERTEX_BUFFER_VIEW vbv = mesh->GetVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW  ibv = mesh->GetIndexBufferView();

		// Set the geometry
		Graphics::CommandList->IASetVertexBuffers(0, 1, &vbv);
		Graphics::CommandList->IASetIndexBuffer(&ibv);

		// Draw
		Graphics::CommandList->DrawIndexedInstanced((UINT)mesh->GetIndexCount(), 1, 0, 0, 0);
	}

	// Transition back buffer to present
	{
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		Graphics::CommandList->ResourceBarrier(1, &rb);
	}

	// Execute and Present
	Graphics::CloseAndExecuteCommandList();
	bool vsync = Graphics::VsyncState();
	Graphics::SwapChain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
	Graphics::AdvanceSwapChainIndex();
	Graphics::ResetAllocatorAndCommandList(Graphics::SwapChainIndex());
}




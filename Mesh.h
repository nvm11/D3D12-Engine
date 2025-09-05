#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
private:
	// Geometry
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};

	//Data Counts
	size_t indices; //index count
	size_t vertices; //vertex count

	//Helper Methods
	//Create buffers from necessary data
	void CreateBuffers(Vertex* vertexData, unsigned int* indexData, size_t vertexCount, size_t indexCount);
	// Calculate tangents for normal mapping
	void CalculateTangents(Vertex* verts, size_t numVerts, unsigned int* indices, size_t numIndices);

public:
	//Constructor
	//Creates buffers and 
	Mesh(Vertex* vertexData, unsigned int* indexData, size_t vertexCount, size_t indexCount);

	Mesh(const char* meshData);

	//Destructor
	//most likely empty, still necessary so ComPtrs clean up
	~Mesh();

	//Methods

	//Getters
	// Returns view to the vertex buffer
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	// Returns the view to the index buffer
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();
	//Returns number of indices
	size_t GetIndexCount() const;
	//Returns number of vertices
	size_t GetVertexCount() const;
};


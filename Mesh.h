#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include "Vertex.h"
class Mesh
{
private:
	//Buffer Pointers
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer; //vertex buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer; //index buffer

	//Data Counts
	unsigned int indices; //index count
	unsigned int vertices; //vertex count

	//Helper Methods
	//Create buffers from necessary data
	void CreateBuffers(Vertex* vertexData, unsigned int* indexData, size_t vertexCount, size_t indexCount);

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
	//Returns the vertex buffer ComPtr
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() const;
	//Returns index buffer ComPtr
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() const;
	//Returns number of indices
	int GetIndexCount() const;
	//Returns number of vertices
	int GetVertexCount() const;

	//Output
	//Sets buffers and draws using indices count
	void Draw();

	//Helpers
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};


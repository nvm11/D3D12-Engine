#pragma once
#include "Mesh.h"
#include "Transform.h"
#include <memory>

class Entity
{
public:
	Entity(std::shared_ptr<Mesh> mesh);
	
	std::shared_ptr<Mesh> GetMesh() const;
	std::shared_ptr<Transform> GetTransform() const;

	void SetMesh(std::shared_ptr<Mesh> mesh);

private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
};


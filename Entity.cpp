#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh) {
	mesh = mesh;
	transform = std::make_shared<Transform>();
}

std::shared_ptr<Mesh> Entity::GetMesh() const {
	return mesh;
}

std::shared_ptr<Transform> Entity::GetTransform() const {
	return transform;
}

void Entity::SetMesh(std::shared_ptr<Mesh> mesh) {
	this->mesh = mesh;
}
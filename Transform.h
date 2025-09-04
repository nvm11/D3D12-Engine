#pragma once
#include <DirectXMath.h>

class Transform
{
private:
	//Fields
	//used in transformations
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT3 pitchYawRoll;	//x, y, z

	//direction vectors
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 forward;

	//world matrix holding transformations
	DirectX::XMFLOAT4X4 world;
	//TODO: Implement
	DirectX::XMFLOAT4X4 worldInverseTranspose;

	//used to track if matrix needs to be remade
	bool isDirty;
	//tracks if the direction vectors need to be remade
	bool isDirtyRotate;

	//Helper Methods
	//remakes matrices and direction vectors
	void RemakeMatrices();
	//Helper that creates a direction vector
	void CreateDirectionVectors();

public:
	//Constructors
	//Default Constructor
	Transform();
	//Destructor
	~Transform();

	//Methods
	//Setters
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 newPos);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 newRot);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 newScale);
	//Getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetForward();

	//Transformations
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 newPos);
	void Rotate(float x, float y, float z);
	void Rotate(DirectX::XMFLOAT3 newRot);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 newScale);
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 posOffset);

};


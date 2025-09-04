#pragma once
#include "Transform.h"
#include "Input.h"

class Camera
{
private:
	//Fields
	//Transformations
	Transform transform;
	//Viewing
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
	//Customization of viewing
	float fovAngle; //in radians
	float nearClipDist;
	float farClipDist;
	float orthWidth; //width of objs for orthographic camera
	bool isPerspective;

public:
	//Constructor
	Camera(DirectX::XMFLOAT3 initialPos,
		float fov = DirectX::XM_PIDIV2,
		float apsectRatio = 0.0f,
		float nearClipDist = 0.01f,
		float farClipDist = 100.0f,
		bool perspective = true,
		float orthographicWidth = 5.0f);
	//Destructor
	~Camera();

	//Methods
	//Getters
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();
	Transform GetTransform();
	float GetFOV();
	//TODO: Add getters and setters for most camera variables

	//Matrix Operations
	//called once per frame
	void UpdateViewMatrix();
	//Called on initialization and when window resizes
	void UpdateProjectionMatrix(float aspectRatio);

	//Game loop
	//processes user input, 
	//adjusts the transform, 
	//and updates view matrix
	void Update(float dt);
};


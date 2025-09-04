#include "Camera.h"
#include <iostream>
using namespace DirectX;

Camera::Camera(XMFLOAT3 initialPos, float fov, float aspectRatio, float nearClipDist, float farClipDist, bool perspective, float orthographicWidth)
	:fovAngle(fov), nearClipDist(nearClipDist), farClipDist(farClipDist), isPerspective(perspective), orthWidth(orthographicWidth)
{
	transform.SetPosition(initialPos);
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera()
{
	//Memory cleanup if needed
}

XMFLOAT4X4 Camera::GetView()
{
	return view;
}

XMFLOAT4X4 Camera::GetProjection()
{
	return projection;
}

Transform Camera::GetTransform()
{
	return transform;
}

void Camera::UpdateProjectionMatrix(float aspect)
{
	//create temp math-type matix
	XMMATRIX projMatrix;

	//check projection type
	if (isPerspective) {
		projMatrix = XMMatrixPerspectiveFovLH(
			fovAngle,
			aspect,
			nearClipDist,
			farClipDist);
	}
	else {
		projMatrix = XMMatrixOrthographicLH(
			orthWidth,
			orthWidth / aspect, //convert projection height into world units
			nearClipDist,
			farClipDist);
	}

	//store result
	XMStoreFloat4x4(&projection, projMatrix);
}

float Camera::GetFOV()
{
	return fovAngle;
}

void Camera::UpdateViewMatrix()
{
	//needs to be done so vars have an address
	XMFLOAT3 pos = transform.GetPosition();
	XMFLOAT3 forward = transform.GetForward();

	XMMATRIX newView = XMMatrixLookToLH(
		XMLoadFloat3(&pos),
		XMLoadFloat3(&forward),
		XMVectorSet(0, 1, 0, 0)); //world up

	XMStoreFloat4x4(&view, newView);
}

void Camera::Update(float dt)
{
	//declare a speed variable
	float speed = dt * 2.0f;

	//Process input
	//Movement
	if (Input::KeyDown('W')) { transform.MoveRelative(0, 0, speed); }
	if (Input::KeyDown('S')) { transform.MoveRelative(0, 0, -speed); }
	if (Input::KeyDown('A')) { transform.MoveRelative(-speed, 0, 0); }
	if (Input::KeyDown('D')) { transform.MoveRelative(speed, 0, 0); }
	if (Input::KeyDown(' ')) { transform.MoveAbsolute(0, speed, 0); }
	if (Input::KeyDown(VK_CONTROL)) { transform.MoveAbsolute(0, -speed, 0); }
	//Rotation
	if (Input::MouseLeftDown()) {
		//get cursor change
		//1.0 is mouse look speed, eventually will be a variable
		float xChange = 0.001f * Input::GetMouseXDelta();
		float yChange = 0.001f * Input::GetMouseYDelta();

		//apply rotation, remeber inverted axes
		transform.Rotate(yChange, xChange, 0);

		//prevent roll by only allowing pi/2 movement
		XMFLOAT3 currentRot = transform.GetRotation();
		if (currentRot.x > XM_PIDIV2) currentRot.x = XM_PIDIV2;
		if (currentRot.x < -XM_PIDIV2) currentRot.x = -XM_PIDIV2;
		transform.SetRotation(currentRot);
	}

	//Update view matrix - do this at end so it is 100% up to date
	UpdateViewMatrix();
}

#pragma once
#include <DirectXMath.h>

//define types of lights
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT	   1
#define LIGHT_TYPE_SPOT		   2

#define MAX_LIGHTS 128

struct Light
{
	//fields
	int type; //type of light ^
	DirectX::XMFLOAT3 direction; //direction for spot and directional lights
	float range; //max range for point and spot lights
	DirectX::XMFLOAT3 position; //position of the light
	float intensity; //how bright is the light
	DirectX::XMFLOAT3 color; //color of the light source
	float spotInnerAngle; //inner angle, in radians (brighter portion)
	float spotOuterAngle; //outer angle in radians (softer light)
	DirectX::XMFLOAT2 padding; //hit 16 byte boundary for cbuffers
};


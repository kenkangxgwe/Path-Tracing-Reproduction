/*
 * Camera.h
*/

#pragma once
#include <sutil.h>
#include <Arcball.h>
using namespace optix;

class Camera
{
public:
	// Camera state
	Camera();
	Camera(float fov, float width, float height);
	Matrix4x4 getFrame();
	Matrix4x4 getLastFrameInverseMat(void);
	void updateCamera(float widht, float height, Context& context);

private:

	float3         p_eye;
	float3         p_lookUp;
	float3         p_lookAt;
	float3         p_u;
	float3         p_v;
	float3         p_w;
	float          p_fieldOfView;
	Matrix4x4      p_rotate;
	Matrix4x4      p_trans;
	Matrix4x4      p_frame;

	Matrix4x4 getTransMat(void);
	Matrix4x4 getRotate(void);
	float getAspectRatio(float width, float height);
	
};
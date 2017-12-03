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
	Camera(float fov, uint32_t width, uint32_t height) ;
	Matrix4x4 getLastFrameInverseMat(void);
	void updateCamera(uint32_t widht, uint32_t height);
	void setEye(float3 newEye);
	float3 getEye(void);
	float3 getLookAt(void);
	void setRotate(Matrix4x4 newRot);

	float3         getU(void);
	float3         getV(void);
	float3         getW(void);
private:

	float3         p_u;
	float3         p_v;
	float3         p_w;
	float3         p_eye;
	float3         p_lookUp;
	float3         p_lookAt;
	float          p_fieldOfView;
	Matrix4x4      p_trans;
	Matrix4x4      p_rotate;
	Matrix4x4      getTransMat(void);
	void           setTransMat(Matrix4x4 frame);
	Matrix4x4      getRotate(void);
	float          getAspectRatio(uint32_t width, uint32_t height);
	
};
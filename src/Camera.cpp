#include "Camera.h"

Camera::Camera()
	: p_eye(make_float3(.0f, 1.0f, 1.0f)),
	  p_lookAt(make_float3(0.0f, 0.0f,-1.0f )),
	  p_lookUp(make_float3(0.0f, 1.0f, 0.0f ))
{
	const float fov= 35.0f;
	uint32_t width = 512;
	uint32_t height = 512;
	// call suil function to calcuate camera u,v,w vectors
		sutil::calculateCameraVariables(
			p_eye, p_lookAt, p_lookUp, p_fieldOfView, getAspectRatio(width, height),
			p_u, p_v, p_w, true);
	p_rotate = Matrix4x4::identity();
}

Camera::Camera(float fov, uint32_t width, uint32_t height)
	: p_eye(make_float3(-0.0f, 1.0f, 5.0f)),
	  p_lookAt(make_float3(-0.0f, 1.0f,-1.04f )),
	  p_lookUp(make_float3(0.0f, 1.0f, 0.0f ))

{
	p_fieldOfView = fov;
	// call suil function to calcuate camera u,v,w vectors
		sutil::calculateCameraVariables(
			p_eye, p_lookAt, p_lookUp, p_fieldOfView, getAspectRatio(width, height),
			p_u, p_v, p_w, true);
	p_rotate = Matrix4x4::identity();
}

float Camera::getAspectRatio(uint32_t width, uint32_t height)
{
	return static_cast<float>(width) / static_cast<float>(height);
}

Matrix4x4 Camera::getLastFrameInverseMat(void)
{
	return p_trans.inverse();
}

void Camera::updateCamera(uint32_t width, uint32_t height)
{
	// normalization 
    const Matrix4x4 frame = Matrix4x4::fromBasis( 
            normalize( p_u ),
            normalize( p_v ),
            normalize( -p_w ),
            p_lookAt);

	setTransMat(frame);

    p_eye    = make_float3( getTransMat() *make_float4( p_eye,    1.0f ) );
    p_lookAt = make_float3( getTransMat() *make_float4( p_lookAt, 1.0f ) );
	p_lookUp = make_float3( getTransMat() *make_float4( p_lookUp,     0.0f ) );

	sutil::calculateCameraVariables(
		p_eye,  p_lookAt, p_lookUp, p_fieldOfView, getAspectRatio(width, height),
		p_u, p_v, p_w, true);

	p_rotate = Matrix4x4::identity();
}

void Camera::setEye(float3 newEye)
{
	p_eye = newEye;
}

float3 Camera::getEye(void)
{
	return p_eye;
}

float3 Camera::getLookAt(void)
{
	return p_lookAt;
}

void Camera::setRotate(Matrix4x4 newRot)
{
	p_rotate = newRot;
}

void Camera::keyIn(MOVE_DIR direction)
{
	float velocity = 3.0f * 0.1;
	if (direction == FORWARD)
	{
		p_eye += p_w* velocity;
		p_lookAt += p_w* velocity;
	}
	if (direction == BACKWARD)
	{
		p_eye-= p_w* velocity;
		p_lookAt -= p_w* velocity;
	}
	if (direction == LEFT)
	{
		p_eye-= p_u* velocity;
		p_lookAt -= p_u* velocity;
	}
	if (direction == RIGHT)
	{
		p_eye+= p_u* velocity;
		p_lookAt += p_u* velocity;
	}
	if (direction == UP)
	{
		p_eye+= p_v* velocity;
		p_lookAt += p_v* velocity;
	}
	if (direction == DOWN)
	{
		p_eye-= p_v* velocity;
		p_lookAt -= p_v* velocity;
	}
}

Matrix4x4 Camera::getTransMat(void)
{
	return p_trans;
}

void Camera::setTransMat(Matrix4x4 frame)
{
	// Apply rotate twice for old SDK for camera.
	p_trans = getRotate()* getRotate();
	//p_trans = frame *getRotate()* getRotate() * frame.inverse(); 

}

Matrix4x4 Camera::getRotate(void)
{
	return p_rotate;
}

float3 Camera::getU(void)
{
	return p_u;
}

float3 Camera::getV(void)
{
	return p_v;
}

float3 Camera::getW(void)
{
	return p_w;
}


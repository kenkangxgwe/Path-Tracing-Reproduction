#include "Camera.h"

Camera::Camera(float fov, float width, float height)
	: p_eye(make_float3(278.0f, 273.0f, -900.0f)),
	  p_lookAt(make_float3(278.0f, 273.0f,0.0f )),
	  p_lookUp(make_float3(0.0f, 1.0f, 0.0f ))

{
	p_fieldOfView = fov;
	sutil::calculateCameraVariables(
            p_eye, p_lookAt, p_lookUp, p_fieldOfView, getAspectRatio(width, height),
            p_u, p_v, p_w, true );
    p_frame = Matrix4x4::fromBasis( 
            normalize( p_u ),
            normalize( p_v ),
            normalize( p_w ),
            p_lookAt);
	p_rotate = Matrix4x4::identity();
}

float Camera::getAspectRatio(float width, float height)
{
	return width / height;
}

Matrix4x4 Camera::getFrame()
{
	return p_frame;
}

Matrix4x4 Camera::getLastFrameInverseMat(void)
{
	return Matrix4x4();
}

void Camera::updateCamera(float width, float height, Context & context)
{
	
    p_eye    = make_float3( getTransMat() *make_float4( p_eye,    1.0f ) );
    p_lookAt = make_float3( getTransMat() *make_float4( p_lookAt, 1.0f ) );
    p_lookUp = make_float3( getTransMat() *make_float4( p_lookUp,     0.0f ) );

    sutil::calculateCameraVariables(
            p_eye, p_lookAt, p_lookUp, p_fieldOfView, getAspectRatio(width, height),
            p_u, p_v, p_w, true );
    context[ "eye"]->setFloat( p_eye );
    context[ "U"  ]->setFloat( p_u );
    context[ "V"  ]->setFloat( p_v );
    context[ "W"  ]->setFloat( p_w );
}

Matrix4x4 Camera::getTransMat(void)
{
	// Apply rotate twice for old SDK for camera.
	return getFrame() *getRotate()* getRotate() *getFrame().inverse(); 

}

Matrix4x4 Camera::getRotate(void)
{
	return p_rotate;
}


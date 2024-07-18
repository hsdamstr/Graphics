#pragma once

#if !defined(MISSING_OVR) && defined(_WIN32)
#include "dispmode.h"
#include "stdio.h"


//#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"

#include <GL/glew.h>



class dispmodeOculusWindows : public dispmode
{
private:
	ovrEyeType dispmodeOculusWindows::get_ovrEye(int viewportID);

public:
	dispmodeOculusWindows();
	~dispmodeOculusWindows();
	virtual viewmat_eye eye_type(int viewportID);
	virtual int num_viewports(void);
	virtual void get_viewport(int viewportValue[4], int viewportId);
	virtual void get_frustum(float result[6], int viewportID);
	virtual void get_projmatrix(float result[6], int viewportID);
	virtual void begin_frame();
	virtual void end_frame();
	virtual void begin_eye(int viewportId);
	virtual void end_eye(int viewportId);
	virtual int get_framebuffer(int viewportID);
	void get_eyeoffset(float offset[3], viewmat_eye eye);

	// Place to store final blitted textures:
	GLuint prerenderDepth[2] = { 0,0 };
	// textureID of color prerender is stored in the TextureChain.
	GLuint prerenderFramebuffer[2];

	// Place to store the temporary MSAA framebuffers which will be blitted.
	GLuint prerenderColorMSAA[2] = { 0,0 };
	GLuint prerenderDepthMSAA[2] = { 0,0 };
	GLuint prerenderFramebufferMSAA[2];
	ovrTextureSwapChain  TextureChain[2];
	ovrSizei idealTextureSize;

	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef      hmdToEyeViewPose[2];
	ovrHmdDesc hmdDesc;

	ovrSession session;
	ovrPosef EyeRenderPose[2];
	double sensorSampleTime;
	long long frameIndex;

	// Mirroring
	ovrMirrorTexture mirrorTexture = nullptr;
	GLuint          mirrorFBO = 0;
};


#endif

#include "windows-compat.h"
#include <stdlib.h>

#include "camcontrol-oculus-windows.h"
#include "sensorfuse.h"
#include "kuhl-util.h"
#include "vecmat.h"
#include "vrpn-help.h"

camcontrolOculusWindows::camcontrolOculusWindows(dispmode *currentDisplayMode, const float initialPos[3])
	:camcontrol(currentDisplayMode)
{
	vec3f_copy(oculusPosition, initialPos);
	oculus = dynamic_cast<dispmodeOculusWindows*>(currentDisplayMode);
}

camcontrolOculusWindows::~camcontrolOculusWindows()
{
}

viewmat_eye camcontrolOculusWindows::get_separate(float pos[3], float rot[16], viewmat_eye requestedEye)
{
	mat4f_identity(rot);
	vec3f_set(pos, 0, 0, 0);

	// Note, if middle is requested, we will return the middle
	// (without using the eye variable) if we are using Vicon. If we
	// are using Oculus tracking, we actually return the left eye.
	ovrEyeType eye = ovrEye_Left;
	if(requestedEye == VIEWMAT_EYE_RIGHT)
		eye = ovrEye_Right;



	// Given eye offsets, get position and orientation.
	// Note: We only update position when left eye is requested so that the left+right eyes are rendered using the same information. If we update the left and right values seperately, then the IPD may not be consistent.
	if(requestedEye == VIEWMAT_EYE_LEFT)
		ovr_GetEyePoses(oculus->session, oculus->frameIndex, ovrTrue, oculus->hmdToEyeViewPose, oculus->EyeRenderPose, &(oculus->sensorSampleTime));


	// Save the position/orientation we got when get_seperate() was called for the left eye so that we can reuse it for the right eye later.
	static float vrpnLeftPos[3], vrpnLeftOrient[16];
	const char *vrpnObject = kuhl_config_get("viewmat.vrpn.object");
	if (vrpnObject != NULL) // Oculus + VRPN orientation
	{
		// If left eye is requested, update vrpn position/orientation.
		if (requestedEye == VIEWMAT_EYE_LEFT)
		{
			float vrpnPos[3], vrpnOrient[16];
			vrpn_get(vrpnObject, NULL, vrpnPos, vrpnOrient);

			// Get the orientation from the Oculus sensor.
			// (Orientation should be same for both eyes)
			float origOrient[16];
			mat4f_rotateQuat_new(origOrient,
				oculus->EyeRenderPose[ovrEye_Left].Orientation.x,
				oculus->EyeRenderPose[ovrEye_Left].Orientation.y,
				oculus->EyeRenderPose[ovrEye_Left].Orientation.z,
				oculus->EyeRenderPose[ovrEye_Left].Orientation.w);

			// Intelligently use both vrpn and oculus orientation.
			sensorfuse(rot, origOrient, vrpnOrient);

			// Copy data so we can use it for when right eye is requested later:
			vec3f_copy(vrpnLeftPos, vrpnPos);
			mat4f_copy(vrpnLeftOrient, rot);
		}


		// Copy the VRPN position
		vec3f_copy(pos, vrpnLeftPos);
		mat4f_copy(rot, vrpnLeftOrient);

		// Eye offset will be retrieved from a subsequent call to dispmodeOculusWindows::get_eyeoffset()
		return VIEWMAT_EYE_MIDDLE;
	}
	else // Oculus-only rotation
	{
		float orientationQuat[4];
		orientationQuat[0] = oculus->EyeRenderPose[eye].Orientation.x;
		orientationQuat[1] = oculus->EyeRenderPose[eye].Orientation.y;
		orientationQuat[2] = oculus->EyeRenderPose[eye].Orientation.z;
		orientationQuat[3] = oculus->EyeRenderPose[eye].Orientation.w;
		mat4f_rotateQuatVec_new(rot, orientationQuat);

		// Note: Eyes rotate around some location if sensor is NOT used.
		pos[0] = oculus->EyeRenderPose[eye].Position.x;
		pos[1] = oculus->EyeRenderPose[eye].Position.y;
		pos[2] = oculus->EyeRenderPose[eye].Position.z;
	}

	// Add translation based on the user-specified initial
	// position. You may choose to initialize the Oculus
	// position to y=1.5 meters to approximate a normal
	// standing eyeheight.
	vec3f_add_new(pos, oculusPosition, pos);


	//printf("eye: %d\n", eye);
	//mat4f_print(rot);
	//vec3f_print(pos);
	return requestedEye;
}


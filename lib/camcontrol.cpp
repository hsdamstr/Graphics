#include "camcontrol.h"
#include "viewmat.h"
#include "vecmat.h"

camcontrol::camcontrol(dispmode *currentDisplayMode)
{
	displaymode = currentDisplayMode;
	vec3f_set(pos, 0,0,0);
	vec3f_set(look, 0,0,-1);
	vec3f_set(up, 0,1,0);
}

camcontrol::camcontrol(dispmode *currentDisplayMode, const float inPos[3], const float inLook[3], const float inUp[3])
{
	displaymode = currentDisplayMode;
	vec3f_copy(pos, inPos);
	vec3f_copy(look, inLook);
	vec3f_copy(up, inUp);
}

/** Gets camera position and a rotation matrix for the camera.
    
    @param outPos The position of the camera.
    
    @param outRot A rotation matrix for the camera.

    @param requestedEye Specifies the eye that we wish to get the
    position/orientation of.
    
    @return The eye that the matrix is actually for. In some cases,
    the requestedEye might NOT match the actual eye. For example, the
    mouse movement manipulator might always return VIEWMAT_EYE_MIDDLE
    regardless of which eye was requested---and the caller must update
    it appropriately. Or, you can use the get() function instead of
    this one which provides the logic to apply the appropriate
    correction.
*/
viewmat_eye camcontrol::get_separate(float outPos[3], float outRot[16], viewmat_eye requestedEye)
{
	mat4f_lookatVec_new(outRot, pos, look, up);

	// Translation will be in outPos, not in the rotation matrix.
	float zero[4] = { 0,0,0,1 };
	mat4f_setColumn(outRot, zero, 3);

	// Invert matrix because the rotation matrix will be inverted
	// again later.
	mat4f_invert(outRot);
		
	vec3f_copy(outPos, pos);
	return VIEWMAT_EYE_MIDDLE;
}

/* Creates a view matrix given a position and rotation. */
void camcontrol::viewmat_from_pos_rot(float matrix[16], float pos[3], float rot[16])
{
	/* Create a translation matrix based on the eye position. Note
	 * that the eye position is negated because we are translating the
	 * camera (or, equivalently, translating the world)---not an
	 * object. */
	float trans[16];
	mat4f_translate_new(trans, -pos[0],-pos[1],-pos[2]);

	/* We invert the rotation matrix because we are rotating the camera, not an object. */
	mat4f_transpose(rot);

	/* Combine into a single view matrix. */
	mat4f_mult_mat4f_new(matrix, rot, trans);
}

/** Gets a view matrix. This function will call a get_separate()
    function which different camera controllers typically implement.
    This get() function will attempt to automatically calculate the
    requested eye even if get_separate() fails to return the requested
    eye.
	    
    @param matrix The requested view matrix.
	    
    @param requestedEye The eye that we are requesting.

    @return The eye that the matrix is actually for. In almost all
    cases the returned value should match the requested eye.
*/
viewmat_eye camcontrol::get(float matrix[16], viewmat_eye requestedEye) {

	/* Get the eye's position and orientation */
	float pos[3], rot[16];
	viewmat_eye actualEye = this->get_separate(pos, rot, requestedEye);

	/* If we got the eye that was requested. */
	if(requestedEye == actualEye)
	{
		this->viewmat_from_pos_rot(matrix, pos, rot);
		return actualEye;
	}

	/* If we requested left/right but we currently have the middle
	 * eye, apply the offset to the middle eye to get the requested
	 * eye. */
	if(actualEye == VIEWMAT_EYE_MIDDLE &&
	   (requestedEye == VIEWMAT_EYE_LEFT ||
	    requestedEye == VIEWMAT_EYE_RIGHT) )
	{
		/* Construct a view matrix for the "middle" eye. */
		this->viewmat_from_pos_rot(matrix, pos, rot);
		
		/* Determine the eye offset we need to apply to actually get
		 * the left or right eye. */
		float eyeOffset[3];
		displaymode->get_eyeoffset(eyeOffset, requestedEye);

		/* Create a new matrix which we will use to translate the
		   existing view matrix. We negate eyeOffset because the
		   matrix would shift the world, not the eye by default. */
		float shiftMatrix[16];
		mat4f_translate_new(shiftMatrix, -eyeOffset[0], -eyeOffset[1], -eyeOffset[2]);
			
		/* Adjust the view matrix by the eye offset */
		mat4f_mult_mat4f_new(matrix, shiftMatrix, matrix);
			
		return requestedEye;
	}

	// If we requested the middle eye but don't get it, return the
	// orientation of one of the eyes and the average of the left and
	// right eye positions.
	if(requestedEye == VIEWMAT_EYE_MIDDLE &&
	   (actualEye == VIEWMAT_EYE_LEFT ||
	    actualEye == VIEWMAT_EYE_RIGHT))
	{
		/* Request the position/orientation of the other eye */
		viewmat_eye otherActualEye;
		float posOther[3], rotOther[16];
		if(actualEye == VIEWMAT_EYE_LEFT)
			otherActualEye = this->get_separate(posOther, rotOther, VIEWMAT_EYE_RIGHT);
		else
			otherActualEye = this->get_separate(posOther, rotOther, VIEWMAT_EYE_LEFT);

		/* If we now have a left and right eye pair, average the position */
		if((actualEye == VIEWMAT_EYE_LEFT &&
		    otherActualEye == VIEWMAT_EYE_RIGHT) ||
		   (actualEye == VIEWMAT_EYE_RIGHT &&
		    otherActualEye == VIEWMAT_EYE_LEFT))
		{
			float posAvg[3];
			vec3f_add_new(posAvg, pos, posOther);
			vec3f_scalarDiv(posAvg, 3.0f);

			this->viewmat_from_pos_rot(matrix, posAvg, rot);
			return VIEWMAT_EYE_MIDDLE;
		}
	}

	/* If we got here, we did not return the requested eye. */
	msg(MSG_WARNING, "camcontrol->get(): You requested eye %d but we are returning eye %d\n", requestedEye, actualEye);
	this->viewmat_from_pos_rot(matrix, pos, rot);
	return actualEye;
}

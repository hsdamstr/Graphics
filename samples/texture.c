/* Copyright (c) 2014-2015 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file Demonstrates drawing textured geometry.
 *
 * @author Scott Kuhl
 */

#include "libkuhl.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static GLuint program = 0; /**< id value for the GLSL program */
static kuhl_geometry triangle;


/* Called by GLFW whenever a key is pressed. */
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;
	/* custom key handling code here */
}

/** Draws the 3D scene. */
void display()
{
	/* Render the scene once for each viewport. Frequently one
	 * viewport will fill the entire screen. However, this loop will
	 * run twice for HMDs (once for the left eye and once for the
	 * right). */
	viewmat_begin_frame();
	for(int viewportID=0; viewportID<viewmat_num_viewports(); viewportID++)
	{
		viewmat_begin_eye(viewportID);

		/* Where is the viewport that we are drawing onto and what is its size? */
		int viewport[4]; // x,y of lower left corner, width, height
		viewmat_get_viewport(viewport, viewportID);
		/* Tell OpenGL the area of the window that we will be drawing in. */
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		/* Clear the current viewport. Without glScissor(), glClear()
		 * clears the entire screen. We could call glClear() before
		 * this viewport loop---but in order for all variations of
		 * this code to work (Oculus support, etc), we can only draw
		 * after viewmat_begin_eye(). */
		glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(.2,.2,.2,0); // set clear color to grey
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST); // turn on depth testing
		kuhl_errorcheck();

		/* Turn on blending (note, if you are using transparent textures,
		   the transparency may not look correct unless you draw further
		   items before closer items. This program always draws the
		   geometry in the same order.). */
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

		/* Get the view or camera matrix; update the frustum values if needed. */
		float viewMat[16], perspective[16];
		viewmat_get(viewMat, perspective, viewportID);

		/* Calculate an angle to rotate the object. glfwGetTime() gets
		 * the time in seconds since GLFW was initialized. Rotates 45 degrees every second. */
		float angle = -90;

		/* Make sure all computers/processes use the same angle */
		/* Create a 4x4 rotation matrix based on the angle we computed. */
		float rotateMat[16];
		mat4f_rotateAxis_new(rotateMat, angle, 1,0,0);

		/* Create a scale matrix. */
		float scaleMatrix[16];
		mat4f_scale_new(scaleMatrix, 3, 3, 3);

		// Modelview = (viewMatrix * scaleMatrix) * rotationMatrix
		float modelview[16];
		mat4f_mult_mat4f_new(modelview, viewMat, scaleMatrix);
		mat4f_mult_mat4f_new(modelview, modelview, rotateMat);

		/* Tell OpenGL which GLSL program the subsequent
		 * glUniformMatrix4fv() calls are for. */
		kuhl_errorcheck();
		glUseProgram(program);
		kuhl_errorcheck();
		
		/* Send the perspective projection matrix to the vertex program. */
		glUniformMatrix4fv(kuhl_get_uniform("Projection"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   perspective); // value
		/* Send the modelview matrix to the vertex program. */
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelview); // value
		kuhl_errorcheck();
		/* Draw the geometry using the matrices that we sent to the
		 * vertex programs immediately above */
		kuhl_geometry_draw(&triangle);

		glUseProgram(0); // stop using a GLSL program.
		viewmat_end_eye(viewportID);
	} // finish viewport loop
	viewmat_end_frame();

	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

}

void init_geometryTriangle(kuhl_geometry *geom, GLuint prog)
{
	

	int numTrisVert = 1;
	int numTrisHorz = 1;
    int numVerts = (numTrisHorz * numTrisVert * 18);
    float triWidth = 1 / ((float) (numTrisHorz));
	float triHeight = 1 / ((float) (numTrisVert));

	kuhl_geometry_new(geom, prog, numTrisVert * numTrisHorz * 6, GL_TRIANGLES);

	/* The data that we want to draw */
	GLfloat* vertexData = malloc(numVerts * sizeof(GLfloat));
	int index = 0;
    for (int i = 0; i < numTrisVert; i++)
    {
		for (int j = 0; j < numTrisHorz; j++){
			vertexData[index] = j * triWidth; 
			vertexData[index + 1] = i * triHeight;
			vertexData[index + 2] = 0;
			vertexData[index + 3] = (j + 1) * triWidth;
			vertexData[index + 4] = i * triHeight;
			vertexData[index + 5] = 0;
			vertexData[index + 6] = (j + 1) * triWidth;
			vertexData[index + 7] = (i + 1) * triHeight;
			vertexData[index + 8] = 0;
			vertexData[index + 9] = j * triWidth;
			vertexData[index + 10] = i * triHeight;
			vertexData[index + 11] = 0;
			vertexData[index + 12] = j * triWidth;
			vertexData[index + 13] = (i + 1) * triHeight;
			vertexData[index + 14] = 0;
			vertexData[index + 15] = (j + 1) * triWidth;
			vertexData[index + 16] = (i + 1) * triHeight;
			vertexData[index + 17] = 0;
			index += 18;
		}
	}
	kuhl_geometry_attrib(geom, vertexData, 3, "in_Position", KG_WARN);

	kuhl_geometry_attrib(geom, vertexData, 3, "in_TexCoord", KG_WARN);

	free(vertexData);

	/* Load the texture. It will be bound to texId */	
	GLuint texId = 0;
	float aspectRatio = kuhl_read_texture_file("../images/worldTopo.jpg", &texId);
	msg(MSG_DEBUG, "Aspect ratio of image is %f\n", aspectRatio); // write message to log.txt
	
	/* Tell this piece of geometry to use the texture we just loaded. */
	kuhl_geometry_texture(geom, texId, "tex", KG_WARN);

	kuhl_errorcheck();
}

int main(int argc, char** argv)
{
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), keyboard);
	// glfwSetFramebufferSizeCallback(window, reshape);

	/* Compile and link a GLSL program composed of a vertex shader and
	 * a fragment shader. */
	program = kuhl_create_program("texture.vert", "texture.frag");
	glUseProgram(program);
	kuhl_errorcheck();

	init_geometryTriangle(&triangle, program);
	
	/* Good practice: Unbind objects until we really need them. */
	glUseProgram(0);

	dgr_init();     /* Initialize DGR based on environment variables. */

	float initCamPos[3]  = {0,0,10}; // location of camera
	float initCamLook[3] = {0,0,0}; // a point the camera is facing at
	float initCamUp[3]   = {0,1,0}; // a vector indicating which direction is up
	viewmat_init(initCamPos, initCamLook, initCamUp);
	
	while(!glfwWindowShouldClose(kuhl_get_window()))
	{
		display();
		kuhl_errorcheck();

		/* process events (keyboard, mouse, etc) */
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}

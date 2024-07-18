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
static GLuint programClouds = 0; /**< id value for the GLSL program */
static kuhl_geometry triangle;
static kuhl_geometry triangleCloud;
static int moving = 0;
static float CamPos[3]  = {3,1.3,0}; // location of camera
static float CamLook[3] = {3,0,-3}; // a point the camera is facing at
static float CamUp[3]   = {0,1,0}; // a vector indicating which direction is up
static int isOn = 0;


void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	/* Action can be press (key down), release (key up), or repeat
	   (key is held down, the delay until the first repeat and the
	   rate at which repeats fire may depend on the OS.)
	   
	   Here, we ignore any press events so the event will only
	   happen when the key is released or it will happen repeatedly
	   when the key is held down.
	*/
	
	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
		moving = !moving;
		glfwSetTime(0);
		//reset values 
		CamPos[0]  = 3;
		CamPos[1] = 1.3;
		CamPos[2] = 0;
		CamLook[0] = 3; 
		CamLook[1] = 0;
		CamLook[2] = -3;
		CamUp[0] = 0;
		CamUp[1] = 1;
		CamUp[2] = 0;
	}

	if(key == GLFW_KEY_N && action == GLFW_PRESS){
		isOn = !isOn;
	}
	


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

		if(moving){
			
			if (glfwGetTime() > 33)
			{
				glfwSetTime(0);
				//reset values 
				CamPos[0]  = 3;
				CamPos[1] = 1.3;
				CamPos[2] = 0;
				CamLook[0] = 3; 
				CamLook[1] = 0;
				CamLook[2] = -3;
				CamUp[0] = 0;
				CamUp[1] = 1;
				CamUp[2] = 0;
			}
			
			else if(glfwGetTime() > 3 && glfwGetTime() <= 8){
			CamPos[2] = -glfwGetTime() /2;
				if(sin(glfwGetTime()/3) < 0){
					CamPos[1] = -sin(glfwGetTime()/3) + .5; 
					}
				else{
					CamPos[1] = sin(glfwGetTime()/3) + .5; 
				}
			CamPos[0] = 3 + (glfwGetTime() - 3) / 4;
			}
			else if(glfwGetTime() > 8 && glfwGetTime() <= 15){
				CamPos[0] = ((3 + (glfwGetTime() - 3) / 4) + (glfwGetTime() - 8) / 2); 
				if(sin(glfwGetTime()/3) < 0){
					CamPos[1] = -sin(glfwGetTime()/3) + .5; 
				}
				else{
					CamPos[1] = sin(glfwGetTime()/3) + .5; 
				}
			}
			else if(glfwGetTime() > 15 && glfwGetTime() < 33){
				CamPos[2] = -4 - (15 - glfwGetTime()) / 3;
				if(sin(glfwGetTime()/3) < 0){
					CamPos[1] = -sin(glfwGetTime()/3) + .5; 
				}
				else{
					CamPos[1] = sin(glfwGetTime()/3) + .5; 
				}
				CamPos[0] = 9.5 - ((glfwGetTime() - 15)) / 2;
				
			}
			else{
				CamPos[2] =  -glfwGetTime() / 2;
				if(sin(glfwGetTime()/3) < 0){
					CamPos[1] = -sin(glfwGetTime()/3) + .5; 
				}
				else{
					CamPos[1] = sin(glfwGetTime()/3) + .5; 
				}
			}

			
			
			
			mat4f_lookatVec_new(viewMat, CamPos, CamLook, CamUp);
		}
		
		else{
			viewmat_get(viewMat, perspective, viewportID);
		}
		

		/* Calculate an angle to rotate the object. glfwGetTime() gets
		 * the time in seconds since GLFW was initialized. Rotates 45 degrees every second. */
		float angle = -90;

		/* Make sure all computers/processes use the same angle */
		/* Create a 4x4 rotation matrix based on the angle we computed. */
		float rotateMat[16];
		mat4f_rotateAxis_new(rotateMat, angle, 1,0,0);

		/* Create a scale matrix. */
		float scaleMatrix[16];
		mat4f_scale_new(scaleMatrix, 10, 5, 5);

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
		glUniform1i(kuhl_get_uniform("isOn"), isOn);
		kuhl_errorcheck();
		/* Draw the geometry using the matrices that we sent to the
		 * vertex programs immediately above */

		kuhl_geometry_draw(&triangle);
		
		glUseProgram(programClouds);

		float modelviewCloud[16];
		mat4f_mult_mat4f_new(modelviewCloud, viewMat, scaleMatrix);

		glUniformMatrix4fv(kuhl_get_uniform("Projection"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   perspective); // value
		/* Send the modelview matrix to the vertex program. */
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelviewCloud); // value
		
		kuhl_errorcheck();
		
		
		kuhl_geometry_draw(&triangleCloud);
		

		glUseProgram(0); // stop using a GLSL program.
		viewmat_end_eye(viewportID);
	} // finish viewport loop
	viewmat_end_frame();

	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

}

void init_surface(kuhl_geometry *geom, GLuint prog){
	int numTrisVert = 1000;
	int numTrisHorz = 1000;
    int numVerts = (numTrisHorz * numTrisVert * 18);
	printf("%d" , numVerts);
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
	GLuint texId1 = 0;
	kuhl_read_texture_file_wrap("../images/worldTopo.png", &texId, GL_REPEAT, GL_REPEAT);
	/* Tell this piece of geometry to use the texture we just loaded. */
	kuhl_geometry_texture(geom, texId, "tex", KG_WARN);

	kuhl_read_texture_file_wrap("../images/planet.png", &texId1, GL_REPEAT, GL_REPEAT);

	kuhl_geometry_texture(geom, texId1, "tex1", KG_WARN);

	kuhl_errorcheck();
}


void init_sky(kuhl_geometry *geom, GLuint prog)
{
	kuhl_geometry_new(geom, prog,
	                  4, // number of vertices
	                  GL_TRIANGLES); // type of thing to draw

	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Below, we provide indices to form
	 * triangles out of these vertices. */
	GLfloat vertexPositions[] = {0, .2, 0,
	                             1, .2, 0,
	                             1, .2, -1,
	                             0, .2, -1 };
	kuhl_geometry_attrib(geom, vertexPositions,
	                     3, // number of components x,y,z
	                     "in_Position", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?

	GLfloat texCoord[] = {0, 0,
	                      1, 0,
	                      1, 1,
	                      0, 1 };
	kuhl_geometry_attrib(geom, texCoord,
	                     2, // number of components x,y,z
	                     "in_TexCoord", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?

	
	/* A list of triangles that we want to draw. "0" refers to the
	 * first vertex in our list of vertices. Every three numbers forms
	 * a single triangle. */
	GLuint indexData[] = { 0, 1, 2,  
	                       0, 2, 3 };
	kuhl_geometry_indices(geom, indexData, 6);

	/* Load the texture. It will be bound to texId */	
	GLuint texId;
	kuhl_read_texture_file_wrap("../images/cloud.png", &texId, GL_REPEAT, GL_REPEAT);
	/* Tell this piece of geometry to use the texture we just loaded. */
	kuhl_geometry_texture(geom, texId, "tex", KG_WARN);

	glBindTexture(GL_TEXTURE_2D, texId);
	
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

	program = kuhl_create_program("terrain.vert", "terrain.frag");
	programClouds = kuhl_create_program("cloud.vert", "cloud.frag");
	glUseProgram(program);
	kuhl_errorcheck();

	init_surface(&triangle, program);
	
	/* Good practice: Unbind objects until we really need them. */
	glUseProgram(programClouds);

	init_sky(&triangleCloud, programClouds);
	
	glUseProgram(0);

	dgr_init();     /* Initialize DGR based on environment variables. */

	
	float initCamPos[3]  = {3,1.3,0}; // location of camera
	float initCamLook[3] = {3,0,-3}; // a point the camera is facing at
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

/* Copyright (c) 2014-2015 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file Demonstrates drawing a shaded 3D triangle.
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

float numModels[3][3];

static kuhl_geometry triangle;
static kuhl_geometry quad;
static kuhl_geometry *hippo = NULL;
static kuhl_geometry *tiger = NULL;
static kuhl_geometry *cow = NULL;

static int isRotating=1;
static int isBobbing=1;


/* Called by GLFW whenever a key is pressed. */
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
	if(action == GLFW_PRESS)
		return;

	/* Custom key handling code below: 
	   For a list of keys, see: https://www.glfw.org/docs/latest/group__keys.html  */
	if(key == GLFW_KEY_SPACE)
	{
		printf("toggling rotation\n");
		isRotating = !isRotating;
	}
	if(key == GLFW_KEY_H){
		printf("hello world\n");
	}

	if(key == GLFW_KEY_U){
		printf("toggling bobbing\n");
		isBobbing = !isBobbing;
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

		/* Get the view matrix and the projection matrix */
		float viewMat[16], perspective[16];
		viewmat_get(viewMat, perspective, viewportID);

		/* Calculate an angle to rotate the object. glfwGetTime() gets
		 * the time in seconds since GLFW was initialized. Rotates 45 degrees every second. */
        float angle;
        if(isRotating){
            angle = fmod(glfwGetTime()*45, 360);
        }
		//specific bob levels for hippo lion and cow 
		float bobLevelH;
		float bobLevelC;
		float bobLevelL;
		if(isBobbing){
			bobLevelH = sin(glfwGetTime()) * .5;
			bobLevelC = sin(glfwGetTime() + 3) * .5;
			bobLevelL = sin(glfwGetTime() + 2) * .5;
		}



		/* Make sure all computers/processes use the same angle */
		dgr_setget("angle", &angle, sizeof(GLfloat));

		/* Create a 4x4 rotation matrix based on the angle we computed. */
		float rotateMat[16];
		mat4f_rotateAxis_new(rotateMat, angle, 0,1,0);
		
		//fix hippo and cow so the are poining in the direction they are moving 
		float fixhippo[16];
		mat4f_rotateAxis_new(fixhippo, 90, 0,1,0);
		float fixcow[16];
		mat4f_rotateAxis_new(fixcow, 180, 0,1,0);

		/* Create a scale matrix. */
		float scaleMat[16];
		mat4f_scale_new(scaleMat, 3, 3, 3);


		/*create translate matrix for all models*/
		float moveHippo[16];
		float moveLion[16];
		float moveCow[16];
		float bobHippo[16];
		float bobCow[16];
		float bobLion[16];

		//move hippo lion and cow onto post 
		mat4f_translate_new(moveHippo, 4, 1, 0);
		mat4f_translate_new(moveLion, 0, 1, 4);
		mat4f_translate_new(moveCow, 0, 1, -4);
	
		//make hippo lion and cow bob on poles
		mat4f_translate_new(bobHippo, 0, bobLevelH, 0);
		mat4f_translate_new(bobCow, 0, bobLevelC, 0);
		mat4f_translate_new(bobLion, 0, bobLevelL, 0);



		/* Last parameter must be NULL, otherwise your program
		   can/will crash. The modelview matrix is (the view matrix) *
		   (the model matrix). Here, we have two matrices in the model
		   matrix, and multiply everything together at once into the
		   modelview matrix.
		   
		   modelview = viewmat * sclaeMat * rotateMat 
		                         ^---model matrix---^
		*/
		float modelview[16];
		float modelviewHippo[16]; //places hippo in scen with specified movements and orientation 
		float modelviewCow[16]; //places cow in scen with specified movements and orientation 
		float modelviewLion[16]; //places lion in scen with specified movements and orientation 

		mat4f_mult_mat4f_many(modelview, viewMat, scaleMat, rotateMat, NULL);
		mat4f_mult_mat4f_many(modelviewHippo, viewMat, scaleMat, rotateMat, moveHippo, fixhippo, bobHippo, NULL);
		mat4f_mult_mat4f_many(modelviewLion, viewMat, scaleMat, rotateMat, moveLion, bobLion, NULL);
		mat4f_mult_mat4f_many(modelviewCow, viewMat, scaleMat, rotateMat, moveCow, fixcow, bobCow, NULL);


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
	

		//place the hippo in the scene
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelviewHippo); // value
		kuhl_geometry_draw(hippo); /* Draw the model */

		//place the lion in the scene
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelviewLion); // value
		kuhl_geometry_draw(tiger);

		//place the cow in the scene
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelviewCow); // value
		kuhl_geometry_draw(cow);
		
		
		//place triangles and quads in the scene
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   modelview); // value
		
		kuhl_errorcheck();
		
		/* Draw the geometry using the matrices that we sent to the
		 * vertex programs immediately above */
		kuhl_geometry_draw(&triangle);
		kuhl_geometry_draw(&quad);

		
		
		/* If we wanted to draw multiple triangles and quads at
		 * different locations, we could call glUniformMatrix4fv again
		 * to change the ModelView matrix and then call
		 * kuhl_geometry_draw() again to draw that object again using
		 * the new model matrix. */

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
	kuhl_geometry_new(geom, prog, 36, // num vertices
	                  GL_TRIANGLES); // primitive type

	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Since no indices are provided, every
	 * three vertex positions form a single triangle.*/
	
    float degToR = M_PI/180; //convert degrees to radians
	
	float verts [12];

	//first point
	verts[0] = (sin(degToR * 0)) * 5;
	verts[1] = (cos(degToR * 0)) * 5;

	verts[2] = (sin(degToR * 60)) * 5;
	verts[3] = (cos(degToR * 60)) * 5;

	verts[4] = (sin(degToR * 120)) * 5;
	verts[5] = (cos(degToR * 120)) * 5;

	verts[6] = (sin(degToR * 180)) * 5;
	verts[7] = (cos(degToR * 180)) * 5;

	verts[8] = (sin(degToR * 240)) * 5;
	verts[9] = (cos(degToR * 240)) * 5;

	verts[10] = (sin(degToR * 300)) * 5;
	verts[11] = (cos(degToR * 300)) * 5;



    GLfloat vertexPositions[] = {0, 0, 0,           
								 verts[0], 0, verts[1], 
	                             verts[2], 0, verts[3],
								 0, 0, 0,			
								 verts[2], 0 ,verts[3],
								 verts[4], 0, verts[5],
								 0, 0, 0,
								 verts[4], 0, verts[5],
								 verts[6], 0, verts[7],
								 0, 0, 0,
								 verts[6], 0, verts[7],
								 verts[8], 0, verts[9],
								 0, 0, 0,
								 verts[8], 0, verts[9],
								 verts[10], 0, verts[11],
								 0, 0, 0,
								 verts[10], 0, verts[11],
								 verts[0] , 0, verts[1],
								 0, 6, 0,           //start roof 
	                             verts[0], 5, verts[1],
	                             verts[2], 5, verts[3],
								 0, 6, 0,
								 verts[2], 5 ,verts[3],
								 verts[4], 5, verts[5],
								 0, 6, 0,
								 verts[4], 5, verts[5],
								 verts[6], 5, verts[7],
								 0, 6, 0,
								 verts[6], 5, verts[7],
								 verts[8], 5, verts[9],
								 0, 6, 0,
								 verts[8], 5, verts[9],
								 verts[10], 5, verts[11],
								 0, 6, 0,
								 verts[10], 5, verts[11],
								 verts[0] , 5, verts[1]};                 
	kuhl_geometry_attrib(geom, vertexPositions, // data
	                     3, // number of components (x,y,z)
	                     "in_Position", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?



	/* The normals for each vertex */
	GLfloat normalData[] = {0, 1, 0,
	                        0, 1, 0,
	                        0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
							0, 1, 0,
	                        0, 1, 0,
	                        0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0, //roof start
	        				0, 1, 0,
	                        0, 1, 0,
	                        0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
							0, 1, 0,
	                        0, 1, 0,
	                        0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
                            0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,
							0, 1, 0,};
	//fill normal with correct roof normal values
	for(int i = 54 ; i < 101 ; i = i + 9){
		//vec 1 and 2 difs 
		float dif1; 
		float dif2;
		float dif3;
		float dif4;
		float dif5;
		float dif6;
		
		//calculate differnce in vertices
		dif1 = 0 - vertexPositions[i + 3]; //vec1x -vec2x
		dif2 = 6 - 5; //vec vec1y - vec2y
		dif3 = 0 - vertexPositions[i + 5]; //vec1z - vec2z
		
		//vec 2 and 3 difs 
		dif4 = 0 - vertexPositions[i + 6]; //vec1x - vec3x
		dif5 = 6 - 5; //vec1y - vec3y
		dif6 = 0 - vertexPositions[i + 8]; //vec1z - vec3z

		float val1 = dif5 * dif3 - dif6 * dif2;
		float val2 = dif6 * dif1 - dif4 * dif3;
		float val3 = dif4 * dif2 - dif5 * dif1;

		float normalval = sqrt((val1 * val1) + (val2 * val2) + (val3 * val3)); 
		
		//normalize 
		normalData[i] = normalData[i + 3] = normalData[i + 6] = val1/normalval; 
		normalData[i + 1] = normalData[i+4] = normalData[i + 7] = val2/normalval; 
		normalData[i + 2] = normalData[i + 5] = normalData [i + 8] = val3/normalval; 
	}
	kuhl_geometry_attrib(geom, normalData, 3, "in_Normal", KG_WARN);
}


/* This illustrates how to draw a quad by drawing two triangles and reusing vertices. */
void init_geometryQuad(kuhl_geometry *geom, GLuint prog)
{
	kuhl_geometry_new(geom, prog,
	                  16, // number of vertices
	                  GL_TRIANGLES); // type of thing to draw

	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Below, we provide indices to form
	 * triangles out of these vertices. */
	GLfloat vertexPositions[] = {3.95, 0, 0,
	                             4, 0, 0,
	                             3.95, 5, 0,
	                             4, 5, 0,
								 0, 0, 3.95,
								 0, 0, 4,
								 0, 5, 3.95,
								 0, 5, 4,
								 -3.95, 0, 0,
	                             -4, 0, 0,
	                             -3.95, 5, 0,
	                             -4, 5, 0,
								  0, 0, -3.95,
								 0, 0, -4,
								 0, 5, -3.95,
								 0, 5, -4};
	kuhl_geometry_attrib(geom, vertexPositions,
	                     3, // number of components x,y,z
	                     "in_Position", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?

	/* The normals for each vertex */
	GLfloat normalData[] = {0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
							0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
							0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
							0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1,
	                        0, 0, 1};
	kuhl_geometry_attrib(geom, normalData, 3, "in_Normal", KG_WARN);
	
	/* A list of triangles that we want to draw. "0" refers to the
	 * first vertex in our list of vertices. Every three numbers forms
	 * a single triangle. */
	GLuint indexData[] = { 0, 1, 2,  
	                       1, 2, 3,
						   4, 5, 6,
						   5, 6, 7,
						   8, 9, 10,
						   9, 10, 11,
						   12, 13, 14,
						   13, 14, 15};
	kuhl_geometry_indices(geom, indexData, 24);

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
	program = kuhl_create_program("triangle-shade.vert", "triangle-shade.frag");

	/* Use the GLSL program so subsequent calls to glUniform*() send the variable to
	   the correct program. */
	glUseProgram(program);
	kuhl_errorcheck();
	/* Set the uniform variable in the shader that is named "red" to the value 1. */
	glUniform1i(kuhl_get_uniform("red"), 0);
	kuhl_errorcheck();
	/* Good practice: Unbind objects until we really need them. */
	glUseProgram(0);

	/* Create kuhl_geometry structs for the objects that we want to
	 * draw. */
	init_geometryTriangle(&triangle, program);
	init_geometryQuad(&quad, program);
	
	float bbox [6];
	hippo = kuhl_load_model("../models/merry/hippo.ply", NULL, program, bbox);
	cow = kuhl_load_model("../models/merry/cow.ply", NULL, program, bbox);
	tiger = kuhl_load_model("../models/merry/lion.ply", NULL, program, bbox);


	dgr_init();     /* Initialize DGR based on config file. */

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
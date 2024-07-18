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
static GLuint programTex = 0;
static float rowNum = 10;
static kuhl_geometry buildingBottom[10][10];
static kuhl_geometry windowBottom[10][10];
static kuhl_geometry buildingTop[10][10];
static kuhl_geometry windowTop[10][10];
static float isComplex [10][10];
static kuhl_geometry ground;
static GLuint texId = 0;
static GLuint usingTexture = 0;
static int isMovingFoward = 0;
static int isMovingBackwards = 0;
static float CamPos[3]  = {12.5,10,30}; // location of camera
static float CamLook[3] = {12.5,1,0}; // a point the camera is facing at
static float CamUp[3]   = {0,1,0}; // a vector indicating which direction is up
static float renderCheck = 0;	
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

	if(action == GLFW_RELEASE){
		isMovingFoward = 0;
		isMovingBackwards = 0;
	}	

	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
		if(isMovingFoward == 0){
			isMovingFoward=!isMovingFoward;
		}
	}
	
	if(key == GLFW_KEY_SPACE && action == GLFW_REPEAT){
		if(isMovingFoward == 0){
			isMovingFoward=!isMovingFoward;
		}
	}
	if(key == GLFW_KEY_B && action == GLFW_PRESS){
		if(isMovingBackwards == 0){
			isMovingBackwards=!isMovingBackwards;
		}
	}
	
	if(key == GLFW_KEY_B && action == GLFW_REPEAT){
		if(isMovingBackwards == 0){
			isMovingBackwards=!isMovingBackwards;
		}
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
		glClearColor(.07,.07,.07,0); // set clear color to grey
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST); // turn on depth testing
		kuhl_errorcheck();

		/* Get the view matrix and the projection matrix */
		float viewMat[16], perspective[16];

	
		viewmat_get(viewMat, perspective, viewportID);

		if(isMovingFoward){
			CamPos[2]-= .1;
			CamLook[2]-= .1;
			renderCheck-= .03333333333;
			rowNum+=.03333333333;
			
		}
		if(isMovingBackwards){
			CamPos[2]+=.1;
			CamLook[2]+=.1;
			renderCheck+= .0333333333;
			rowNum-= .033333333333;
		}
		

        mat4f_lookatVec_new(viewMat, CamPos, CamLook, CamUp);
		
		

		


		/* Create a scale matrix. */
		float scaleMat[16];
		mat4f_scale_new(scaleMat, 3, 3, 3);


		/* Last parameter must be NULL, otherwise your program
		   can/will crash. The modelview matrix is (the view matrix) *
		   (the model matrix). Here, we have two matrices in the model
		   matrix, and multiply everything together at once into the
		   modelview matrix.
		   
		   modelview = viewmat * sclaeMat * rotateMat 
		                         ^---model matrix---^ */
	
		kuhl_errorcheck();
		glUseProgram(program);
		kuhl_errorcheck();
		
		/* Send the perspective projection matrix to the vertex program. */
		glUniformMatrix4fv(kuhl_get_uniform("Projection"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   perspective); // value
		/* Send the modelview matrix to the vertex program. */
		float modelview[16];
		for(int i=0; i < 10; i++)
		{
			for(int j=0; j < rowNum; j++)
            {

			
			float randomPosMat[16];
			mat4f_translate_new(randomPosMat, i, 0, -j+10);


			mat4f_mult_mat4f_many(modelview, viewMat, scaleMat, randomPosMat, NULL);

			/* Send the modelview matrix to the vertex program. */
			glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
			                   1, // number of 4x4 float matrices
			                   0, // transpose
			                   modelview); // value

			kuhl_errorcheck();
			if(isComplex[i][j] == 1){
				kuhl_geometry_draw(&buildingTop[i][j]); 
				kuhl_geometry_draw(&windowTop[i][j]);
				kuhl_errorcheck();
			}
			kuhl_geometry_draw(&buildingBottom[i][j]); 
			kuhl_geometry_draw(&windowBottom[i][j]);
			kuhl_errorcheck();
			}
		}
		glUseProgram(0); // stop using a GLSL program.
		glUseProgram(programTex);

		float translateGround[16];
		//printf("%d", renderCheck);
		mat4f_translate_new(translateGround, 0, 0, 3 *floor(renderCheck));
		float modelViewRoad[16];
		mat4f_mult_mat4f_many(modelViewRoad, viewMat, translateGround, scaleMat, NULL);
		glUniformMatrix4fv(kuhl_get_uniform("Projection"),
		                   1, // number of 4x4 float matrices
		                   0, // transpose
		                   perspective); // value
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
			                   1, // number of 4x4 float matrices
			                   0, // transpose
			                   modelViewRoad); // value
		kuhl_geometry_draw(&ground);

		/* If we wanted to draw multiple triangles and quads at
		 * different locations, we could call glUni		   call
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

float randomNum(float min, float max){
	float num = min + drand48()*(max - min);
	return num;
}

void getBuidingDims(float* width, float* widthComp, float* height, float* heightComp , float* depth, float* depthComp, int iscomplex){
	if (iscomplex == 0){
		*width = randomNum(.5 , .8);  
		*height = randomNum(.3, 4);
		*depth = randomNum(.5 , .9);
	}
	else{
		*widthComp = *width - .2; 
		*heightComp = randomNum(.5, 1);
		*depthComp = *depth - .2;
	}
	return;
}

void init_building_bottom(kuhl_geometry *geom, GLuint prog, float width, float height, float depth){
	kuhl_geometry_new(geom, prog,
	                  8, // number of vertices
	                  GL_TRIANGLES); // type of thing to draw

	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Below, we provide indices to form
	 * triangles out of these vertices. */
	GLfloat vertexPositions[] = {0, 0, 0,
	                             -width, 0, 0,
	                             -width, height, 0,
	                             0, height, 0,
								 -width, 0, -depth,
								 -width, height, -depth,
								 0, 0, -depth,
								 0, height, -depth};
	kuhl_geometry_attrib(geom, vertexPositions,
	                     3, // number of components x,y,z
	                     "in_Position", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?

	/* A list of triangles that we want to draw. "0" refers to the
	 * first vertex in our list of vertices. Every three numbers forms
	 * a single triangle. */
	GLuint indexData[] = { 0, 3, 7,  
	                       0, 6, 7,
						   1, 2, 5,
						   1, 4, 5,
						   0, 2, 3,
						   0, 1, 2,
						   3, 2, 5,
                           3, 7, 5};
	kuhl_geometry_indices(geom, indexData, 24);

	GLfloat colorData[] = {.05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05};

	kuhl_geometry_attrib(geom, colorData, 3, "in_Color", KG_WARN);

	kuhl_errorcheck();
}

void init_building_top(kuhl_geometry *geom, GLuint prog, float width, float widthComp , float height, float heightComp, float depth, float depthComp)
{
	kuhl_geometry_new(geom, prog,
	                  8, // number of vertices
	                  GL_TRIANGLES); // type of thing to draw


	
	float fixWidth = (width - widthComp) / 2;
	float fixDepth = (depth - depthComp) / 2;


	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Below, we provide indices to form
	 * triangles out of these vertices. */
	GLfloat vertexPositions[] = {-fixWidth, height, -fixDepth,
	                             -width + fixWidth, height, -fixDepth,
	                             -width + fixWidth, height + heightComp, -fixDepth,
	                             -fixWidth, height + heightComp, -fixDepth,
								 -width + fixWidth, height, -depth + fixDepth,
								 -width + fixWidth, height + heightComp, -depth + fixDepth,
								 -fixWidth, height, -depth + fixDepth,
								 -fixWidth, height + heightComp, -depth + fixDepth};
	kuhl_geometry_attrib(geom, vertexPositions,
	                     3, // number of components x,y,z
	                     "in_Position", // GLSL variable
	                     KG_WARN); // warn if attribute is missing in GLSL program?

	/* A list of triangles that we want to draw. "0" refers to the
	 * first vertex in our list of vertices. Every three numbers forms
	 * a single triangle. */
	GLuint indexData[] = { 0, 3, 7,  
	                       0, 6, 7,
						   1, 2, 5,
						   1, 4, 5,
						   0, 2, 3,
						   0, 1, 2,
						   3, 2, 5,
                           3, 7, 5};
	kuhl_geometry_indices(geom, indexData, 24);

	GLfloat colorData[] = {.05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05,
						   .05,.05,.05};

	kuhl_geometry_attrib(geom, colorData, 3, "in_Color", KG_WARN);

	kuhl_errorcheck();
}

void faceWin(GLfloat* vertexPositions, int* index, float winSpace, float winDim, float antiZFight, int numWinHeight, int numWinWidth, float heightComp ,float displace, float displacez ){
	for (int i = 0; i < numWinHeight; i++){
		for (int j = 0; j < numWinWidth; j++){
			vertexPositions[*index] =  -winSpace  + (j * -winDim) - displace; //x
			vertexPositions[*index+1] = (winSpace + (i * winDim)) + heightComp; //y
			vertexPositions[*index+2] = antiZFight - displacez; //z
			vertexPositions[*index+3] = -winSpace + (j * -winDim) - displace; //x
			vertexPositions[*index+4] = (((i * winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+5] = antiZFight - displacez; //z
			vertexPositions[*index+6] = ((j * -winDim) - winDim) - displace; //x
			vertexPositions[*index+7] = (((i *winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+8] = antiZFight - displacez; //z
			vertexPositions[*index+9] = ((j * -winDim) - winDim) - displace; //x
			vertexPositions[*index+10] = (((i * winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+11] = antiZFight - displacez; //z
			vertexPositions[*index+12] = ((j * -winDim) -winDim) - displace; //x
			vertexPositions[*index+13] = (winSpace + (i * winDim)) + heightComp; //y
			vertexPositions[*index+14] = antiZFight - displacez; //z
			vertexPositions[*index+15] = -winSpace + (j * -winDim) - displace; //x
			vertexPositions[*index+16] = (winSpace + (i * winDim)) + heightComp; //y
			vertexPositions[*index+17] = antiZFight - displacez; //z
			*index += 18;
		}
	}
}
void sideWin(GLfloat* vertexPositions, int* index, float winSpace, float winDim, float antiZFight, int numWinHeight, int numWinDepth, float heightComp, float displace){
	for (int i = 0; i < numWinHeight; i++){
		for (int j = 0; j < numWinDepth; j++){
			vertexPositions[*index] = antiZFight; //x
			vertexPositions[*index+1] = (winSpace + (i *winDim)) + heightComp; //y
			vertexPositions[*index+2] = -winSpace + (j * -winDim) + -displace; //z
			vertexPositions[*index+3] = antiZFight; //x
			vertexPositions[*index+4] = (((i*winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+5] = -winSpace + (j * -winDim) + -displace; //z
			vertexPositions[*index+6] = antiZFight; //x
			vertexPositions[*index+7] = (((i *winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+8] = ((j * -winDim) - winDim) + -displace; //z
			vertexPositions[*index+9] = antiZFight; //x
			vertexPositions[*index+10] = (winSpace + (i *winDim)) + heightComp; //y
			vertexPositions[*index+11] = -winSpace + (j * -winDim) + -displace; //z
			vertexPositions[*index+12] = antiZFight; //x
			vertexPositions[*index+13] = (((i *winDim) + winDim)) + heightComp; //y
			vertexPositions[*index+14] = ((j * -winDim) - winDim) + -displace; //z
			vertexPositions[*index+15] = antiZFight; //x
			vertexPositions[*index+16] = (winSpace + (i *winDim)) + heightComp; //y
			vertexPositions[*index+17] = ((j * -winDim) - winDim) + -displace; //z
			*index += 18;
		}
	}
}

void fillColor(GLfloat* colorData, int verts){
		for(int i = 0; i < verts - 17; i += 18){
		if(drand48() < .2){
			colorData[i] =1; //red
			colorData[i+1] =.8431; //green
			colorData[i+2] = .1; //blue
			colorData[i+3] =1; //red
			colorData[i+4] =.8431; //green
			colorData[i+5] = .1; //blue
			colorData[i+6] =1; //red
			colorData[i+7] =.8431; //green
			colorData[i+8] = .1; //blue
			colorData[i+9] =1; //red
			colorData[i+10] =.8431; //green
			colorData[i+11] = .1; //blue
			colorData[i+12] =1; //red
			colorData[i+13] =.8431; //green
			colorData[i+14] = .1; //blue
			colorData[i+15] =1; //red
			colorData[i+16] =.8431; //green
			colorData[i+17] = .1; //blue
		}
		else{
			colorData[i] = 0; //red
			colorData[i+1] =.0; //greenprintf("im in thrid function");
			colorData[i+2] = .05; //blue
			colorData[i+3] =0; //red
			colorData[i+4] =0; //green
			colorData[i+5] = .05; //blue
			colorData[i+6] =0; //red
			colorData[i+7] =0; //green
			colorData[i+8] = .05; //blue
			colorData[i+9] =0; //red
			colorData[i+10] =0; //green
			colorData[i+11] = .05; //blue
			colorData[i+12] =0; //red
			colorData[i+13] =0; //green
			colorData[i+14] = .05; //blue
			colorData[i+15] =0; //red
			colorData[i+16] =0; //green
			colorData[i+17] = .05; //blue
		}
	}
}

void init_window_bottom(kuhl_geometry *geom, GLuint prog, float width, float height, float depth){
	float winDim = .1; //interchangable window dimensions
	float winSpace = .01;
	
	int numWinWidth = width / winDim; //number of widows across the x coordinate of a building
	int numWinHeight = height / winDim; //number of windows across the y coordinate on a building
	int numWinDepth = depth / winDim; //numer of windows across the z coordinate on a building
	
	int verts = (((numWinWidth * numWinHeight) + ((numWinDepth * numWinHeight) * 2))) * 18; // total number of vertices ill be creating per building
	
	kuhl_geometry_new(geom, prog, (((numWinWidth * numWinHeight) + ((numWinDepth * numWinHeight) * 2))) * 6, GL_TRIANGLES);

	GLfloat* vertexPositions = malloc(sizeof(GLfloat) * verts); //passing ito function need to malloc

	int index = 0; //keep track of where we are located in vertexPositions

	faceWin(vertexPositions, &index, winSpace, winDim, .01, numWinHeight, numWinWidth , 0, 0, 0); //function to draw the windows on the face of a building
	sideWin(vertexPositions, &index, winSpace, winDim, .01, numWinHeight, numWinDepth, 0, 0); //functions to draw the windows on either side of a building
	sideWin(vertexPositions, &index, winSpace, winDim, -width - .01, numWinHeight, numWinDepth, 0, 0);

	kuhl_geometry_attrib(geom, vertexPositions, 3, "in_Position", KG_WARN); // warn if attribute is missing in GLSL program?
	free(vertexPositions);
	
	GLfloat* colorData = malloc(sizeof(GLfloat) * verts);


	fillColor(colorData, verts); //assign colors to windows

	kuhl_geometry_attrib(geom, colorData, 3, "in_Color", KG_WARN);
	free(colorData);
	
	kuhl_errorcheck();
}

void init_window_top(kuhl_geometry *geom, GLuint prog, float width, float depth, float height, float widthComp, float heightComp, float depthComp){
	GLfloat* vertexPositions;
	
	float winDim = .1;
	float winSpace = .01;
	
	int numWinWidth = widthComp/ winDim;
	int numWinHeight = heightComp / winDim;
	int numWinDepth = depthComp / winDim;
	int verts = (((numWinWidth * numWinHeight) + ((numWinDepth * numWinHeight) * 2))) * 18;

	float fixWidth = (width - widthComp) / 2;
	float fixDepth = (depth - depthComp) / 2;	
	
	
	kuhl_geometry_new(geom, prog, (((numWinWidth * numWinHeight) + ((numWinDepth * numWinHeight) * 2))) * 6, GL_TRIANGLES);


	/* Vertices that we want to form triangles out of. Every 3 numbers
	 * is a vertex position. Below, we provide indices to form
	 * triangles out of these vertices. */

	vertexPositions = malloc(sizeof(GLfloat) * verts);

	
	int index = 0;
	faceWin(vertexPositions, &index, winSpace, winDim, .01, numWinHeight, numWinWidth, height , fixWidth , fixDepth);
	sideWin(vertexPositions, &index, winSpace, winDim, -width + fixWidth - .01, numWinHeight, numWinDepth, height, fixDepth);
	sideWin(vertexPositions, &index, winSpace, winDim, -fixWidth + .01, numWinHeight, numWinDepth, height, fixDepth);

	kuhl_geometry_attrib(geom, vertexPositions, 3, "in_Position", KG_WARN); // warn if attribute is missing in GLSL program?
	free(vertexPositions);
	GLfloat* colorData = malloc(sizeof(GLfloat) * verts);


	fillColor(colorData, verts);

	kuhl_geometry_attrib(geom, colorData, 3, "in_Color", KG_WARN);
	free(colorData);
	kuhl_errorcheck();
}

void init_ground(kuhl_geometry *geom, GLuint prog){

	kuhl_geometry_new(geom, prog, 4,GL_TRIANGLES);

	GLfloat vertexPositions[] = {-1,0,0,
								 9,0,0,
								 -1,0,9,
								 9,0,9};
	kuhl_geometry_attrib(geom, vertexPositions, 3, "in_Position", KG_WARN);


	GLuint indexData[] = { 0, 2, 3,  
	                       0, 1, 3};
	kuhl_geometry_indices(geom, indexData, 6);

	GLfloat texCoord[] = {0, 0,
	                      10, 0,
	                       0, 10,
	                      10, 10};
	kuhl_geometry_attrib(geom, texCoord, 2, "in_TexCoord",KG_WARN);


	kuhl_read_texture_file_wrap("../images/road.png", &texId, GL_REPEAT, GL_REPEAT);
	usingTexture = texId;

	kuhl_geometry_texture(geom, usingTexture, "tex", KG_WARN);
	glBindTexture(GL_TEXTURE_2D, texId);

	kuhl_errorcheck();
}


int main(int argc, char** argv){
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), keyboard);
	// glfwSetFramebufferSizeCallbacstatic GLuint texId = 0;
	program = kuhl_create_program("infinicity.vert", "infinicity.frag");
	programTex = kuhl_create_program("texture.vert", "texture.frag");

	/* Use the GLSL program so subsequent calls to glUniform*() send the variable to
	   the correct program. */
	glUseProgram(program);
	kuhl_errorcheck();
	/* Set the uniform variable in the shader that is named "red" to the value 1. */
	kuhl_errorcheck();

	/* Create kuhl_geometry structs for the objects that we want to
	 * draw. */

	float width = 0;
	float height = 0;
	float depth = 0;
	float heightComp = 0;
	float depthComp = 0;
	float widthComp = 0;
	



	for(int i = 0; i < 10; i++){
		for(int j = 0; j < rowNum; j++){
			srand48((123.456 * j) + (9876.543 * i));
			getBuidingDims(&width, &widthComp, &height, &heightComp, &depth, &depthComp, 0);
			init_building_bottom(&buildingBottom[i][j], program, width, height, depth);	
			init_window_bottom(&windowBottom[i][j], program, width, height, depth);
			isComplex[i][j] = 0;
			if(drand48() < .5){
				getBuidingDims(&width, &widthComp, &height, &heightComp, &depth, &depthComp,  1);
				init_building_top(&buildingTop[i][j], program, width, widthComp , height, heightComp, depth, depthComp); 
				init_window_top(&windowTop[i][j], program, width, depth, height, widthComp, heightComp, depthComp);
				isComplex[i][j] = 1; 
			}
			
		} 
	}


	init_ground(&ground, programTex);

	glUseProgram(0); // stop using a GLSL program.
	
	dgr_init();     /* Initialize DGR based on config file. */

	float initCamPos[3]  = {12.5,10,30}; // location of camera
	float initCamLook[3] = {12.5,1,0}; // a point the camera is facing at
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

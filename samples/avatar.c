/* Copyright (c) 2014-2015 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file Loads 3D model files displays them.
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

static int renderStyle = 0;

static kuhl_geometry *fpsgeom = NULL;
static kuhl_geometry *modelgeom  = NULL;
static kuhl_geometry *origingeom = NULL;

/** The following variable toggles the display an "origin+axis" marker
 * which draws a small box at the origin and draws lines of length 1
 * on each axis. Depending on which matrices are applied to the
 * marker, the marker will be in object, world, etc coordinates. */
static int showOrigin=0; // was --origin option used?


/** Initial position of the camera. 1.55 is a good approximate
 * eyeheight in meters.*/
static const float initCamPos[3]  = {0.0f,1.55f,0.0f};

/** A point that the camera should initially be looking at. If
 * --fit is used, this will also be the position that model will be
 * translated to. */
static const float initCamLook[3] = {0.0f,0.0f,-5.0f};

/** A vector indicating which direction is up. */
static const float initCamUp[3]   = {0.0f,1.0f,0.0f};


#define GLSL_VERT_FILE "avatar.vert"
#define GLSL_FRAG_FILE "avatar.frag"

/* Called by GLFW whenever a key is pressed. */
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	if(action != GLFW_PRESS)
		return;
	
	switch(key)
	{
		case GLFW_KEY_R:
		{
			// Reload GLSL program from disk
			kuhl_delete_program(program);
			program = kuhl_create_program(GLSL_VERT_FILE, GLSL_FRAG_FILE);
			/* Apply the program to the model geometry */
			kuhl_geometry_program(modelgeom, program, KG_FULL_LIST);
			/* and the fps label*/
			kuhl_geometry_program(fpsgeom, program, KG_FULL_LIST);

			break;
		}
		// Toggle different sections of the GLSL fragment shader
		case GLFW_KEY_SPACE:
		case GLFW_KEY_PERIOD:
			renderStyle++;
			if(renderStyle > 6)
				renderStyle = 0;
			switch(renderStyle)
			{
				case 0: printf("Render style 0: Diffuse (headlamp light)\n"); break;
				case 1: printf("Render style 1: Vertex color + diffuse (headlamp light)\n"); break;
				case 2: printf("Render style 2: Normals\n"); break;
				case 3: printf("Render style 3: Vertex color\n"); break;
				case 4: printf("Render style 4: Texture (color is used on non-textured geometry)\n"); break;
				case 5: printf("Render style 5: Texture coordinates\n"); break;
				case 6: printf("Render style 6: Texture+diffuse (color is used on non-textured geometry)\n"); break;
			}
			break;
	}
}



/** Draws the 3D scene. */
void display()
{
	/* Display FPS if we are a DGR master OR if we are running without DGR. */
	if(dgr_is_master())
	{
		static long lasttime = 0;
		long now = kuhl_milliseconds();
		if(lasttime == 0 || now - lasttime > 200) // reduce number to increase frequency of FPS label updates.
		{
			lasttime = now;
			
			float fps = bufferswap_fps(); // get current fps
			char message[1024];
			snprintf(message, 1024, "FPS: %0.2f", fps); // make a string with fps in it
			float labelColor[3] = { 1.0f,1.0f,1.0f };
			float labelBg[4] = { 0.0f,0.0f,0.0f,.3f };

			// If DGR is being used, only display FPS info if we are
			// the master process.
			fpsgeom = kuhl_label_geom(fpsgeom, program, NULL, message, labelColor, labelBg, 24);
		}
	}
	
	/* Ensure the slaves use the same render style as the master
	 * process. */
	dgr_setget("style", &renderStyle, sizeof(int));

	
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
		glClearColor(.2f,.2f,.2f,0.0f); // set clear color to grey
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST); // turn on depth testing
		kuhl_errorcheck();

		/* Turn on blending (note, if you are using transparent textures,
		   the transparency may not look correct unless you draw further
		   items before closer items.). */
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

		/* Get the view or camera matrix; update the frustum values if needed. */
		float viewMat[16], perspective[16];
		viewmat_get(viewMat, perspective, viewportID);

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
		                   viewMat); // value

		glUniform1i(kuhl_get_uniform("renderStyle"), renderStyle);

		kuhl_errorcheck();
		kuhl_geometry_draw(modelgeom); /* Draw the model */
		kuhl_errorcheck();
		if(showOrigin && origingeom != NULL)
		{
			/* Save current line width */
			GLfloat origLineWidth;
			glGetFloatv(GL_LINE_WIDTH, &origLineWidth);
			GLfloat lineWidthRange[2] = { -1.0f, -1.0f };
			glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, lineWidthRange);
			if(lineWidthRange[1] > 4)
				glLineWidth(4); // make lines thick
			else
				glLineWidth(lineWidthRange[1]);

			/* Object coordinate system origin */
			kuhl_geometry_draw(origingeom); /* Draw the origin marker */

			/* World coordinate origin */
			glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
			                   1, // number of 4x4 float matrices
			                   0, // transpose
			                   viewMat); // value
			kuhl_geometry_draw(origingeom); /* Draw the origin marker */
			
			/* Restore line width */
			glLineWidth(origLineWidth);
		}

		// aspect ratio will be zero when the program starts (and FPS hasn't been computed yet)
		if(dgr_is_master())
		{
			float labelHeight = 1/16.0f; // height (percentage of window height)
			float labelPadding = 0.01f;  // space around label
				
			float stretchLabel[16];
			mat4f_scale_new(stretchLabel,
			                labelHeight / viewmat_window_aspect_ratio(),
			                labelHeight, 1.0f);
			
			/* Position label in the upper left corner of the screen */
			float transLabel[16];
			mat4f_translate_new(transLabel, -1+labelPadding,
			                    1-labelHeight-labelPadding, 0.0f);
			float modelview[16];
			mat4f_mult_mat4f_new(modelview, transLabel, stretchLabel);
			glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, modelview);

			/* Make sure we don't use a projection matrix */
			float identity[16];
			mat4f_identity(identity);
			glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, identity);

			/* Don't use depth testing and make sure we use the texture
			 * rendering style */
			glDisable(GL_DEPTH_TEST);
			glUniform1i(kuhl_get_uniform("renderStyle"), 1);
			kuhl_geometry_draw(fpsgeom); /* Draw the quad */
			glEnable(GL_DEPTH_TEST);
			kuhl_errorcheck();
		}

		glUseProgram(0); // stop using a GLSL program.
		viewmat_end_eye(viewportID);
	} // finish viewport loop

	/* Update the model for the next frame based on the time. We
	 * convert the time to seconds and then use mod to cause the
	 * animation to repeat. */
	double time = glfwGetTime();
	dgr_setget("time", &time, sizeof(double));
	kuhl_update_model(modelgeom, 1, fmod(time,2));

	viewmat_end_frame();

	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

	//kuhl_video_record("videoout", 30);
}



int main(int argc, char** argv)
{
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);
	
	
	char *modelFilename    = NULL;
	char *modelTexturePath = NULL;

	int currentArgIndex = 1; // skip program name
	int usageError = 0;
	int fitToView = 0;
	while(argc > currentArgIndex)
	{
		if(strcmp(argv[currentArgIndex], "--fit") == 0)
			fitToView = 1;
		else if(strcmp(argv[currentArgIndex], "--origin") == 0)
			showOrigin = 1;
		else if(modelFilename == NULL)
		{
			modelFilename = argv[currentArgIndex];
			modelTexturePath = NULL;
		}
		else if(modelTexturePath == NULL)
			modelTexturePath = argv[currentArgIndex];
		else
		{
			usageError = 1;
		}
		currentArgIndex++;
	}

	if (argc == 1) // no arguments
	{
		msg(MSG_WARNING, "No arguments, defaulting to loading duck.dae with --fit option.");
		modelFilename = "../models/duck/duck.dae";
		fitToView = 1;
		usageError = 0;
	}

	// If we have no model to load or if there were too many arguments.
	if(modelFilename == NULL || usageError)
	{
		printf("Usage:\n"
		       "%s [--fit] [--origin] modelFile     - Textures are assumed to be in the same directory as the model.\n"
		       "- or -\n"
		       "%s [--fit] [--origin] modelFile texturePath\n"
		       "If the optional --fit parameter is included, the model will be scaled and translated to fit within the approximate view of the camera\n"
		       "If the optional --origin parameter is included, a box will is drawn at the origin and unit-length lines are drawn down each axis.\n",
		       argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}


	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), keyboard);
	// glfwSetFramebufferSizeCallback(window, reshape);

	/* Compile and link a GLSL program composed of a vertex shader and
	 * a fragment shader. */
	program = kuhl_create_program(GLSL_VERT_FILE, GLSL_FRAG_FILE);

	dgr_init();     /* Initialize DGR based on environment variables. */

	viewmat_init(initCamPos, initCamLook, initCamUp);

	// Clear the screen while things might be loading
	glClearColor(.2f,.2f,.2f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Load the model from the file
	float bbox[6];
	modelgeom = kuhl_load_model(modelFilename, modelTexturePath, program, bbox);

	// Modify the GeomTransform matrix in the geometry object so that
	// the object fits in a 1x1x1 box sitting on top of the location
	// the camera is looking.
	if(fitToView)
		kuhl_make_geom_fit(modelgeom, bbox, 1, initCamLook[0], initCamLook[1], initCamLook[2]);
	if(showOrigin)
		origingeom = kuhl_load_model("../models/origin/origin.obj", modelTexturePath, program, NULL);


	while(!glfwWindowShouldClose(kuhl_get_window()))
	{
		display();
		kuhl_errorcheck();

		/* process events (keyboard, mouse, etc) */
		glfwPollEvents();
	}
	exit(EXIT_SUCCESS);
}

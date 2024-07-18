/* Copyright (c) 2014-2017 Scott Kuhl. All rights reserved.
* License: This code is licensed under a 3-clause BSD license. See
* the file named "LICENSE" for a full copy of the license.
*/

/** @file Distance judgment experiment.
*
* @author Scott Kuhl, Bochao Li
*/

#include "libkuhl.h"

// mkdir()
#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <time.h>

int blank_screen = 0; // should screen be blank?

typedef enum { EXP_START=0,       // starting experiment, no target shown.
               EXP_SHOW_TARGET=1, // show target at start of trial
               EXP_WALK=2,        // blank screen so they can walk to target
               EXP_RECORDED=3,    // record distance, EVENT_SHOW_TARGET will be next
               EXP_FINISHED=4     // experiment complete, no target shown.
} expState;
expState state = EXP_START;

/* State machine diagram. Arrows point in the possible "forward"
   transitions through the state machine.

                [Start]
                 | Transition: Show target.
                 v
              +>[Show target]
              |  | Transition: Blank screen, record starting position.
Transition:   |  v
  Unblank     | [Walk]
  screen,     |  | Transition: Record position to file.
show target,  |  v
rec start pos +-[Recorded]
                 | Transition: Unblank screen and hide target.
                 v
                [Finished]
*/  


typedef struct {
	float distance;
	int isPractice;
	kuhl_geometry *target;
	float roomOffset;
} trial;

trial *trials = NULL;

/* The trial that we are currently displaying (first trial is 0). */
int current_trial = 0;
/* Total number of trials in the experiment. Set when program
 * starts. */
int num_trials = 0;

/* Subject ID entered by user when the program begins. */
int subjectID = 0;

// Information about how to place the room (does not impact
// target). These values can be changed with different keys.
float rotOffset = 126;
float posOffset[3] = { -3.9, 0.0f, 1.5 };

// Should we show the target? We don't show it when the program starts
// or when the experiment is finished.
int showing_target = 0;

// Text file where we record our data
FILE *output_file = NULL;

// Location in world coordinates for the person to stand and view the targets.
float start_pos[3] = { 1, 0, 0 };
float actual_start_pos[3] = {0,0,0};


// A vector in world coordinates pointing from the starting point in
// the direction that the targets should appear. Should be normalized
// (but is also normalized in main() on startup).
float target_vector[3] = { 0.841, 0, 0.542 };

int debug_mode = 0;

GLuint program = 0; /**< id value for the GLSL program */

kuhl_geometry *modelgeom = NULL;
kuhl_geometry *origingeom = NULL;


/* Converts a world coordinate into a coordinate system for the
 * experiment. Origin=start_pos, X=right, Y=up, -Z=target direction */
float worldToExpMat[16];





//get vrpn curret head position
void get_vrpn_pos_current(float pos[3])
{
#if 0
	float vrpnpos[3], vrpnorient[16];
	vrpn_get("DK2", "vrpnhost", vrpnpos, vrpnorient);

	pos[0] = vrpnpos[0];
	pos[1] = 0;
	pos[2] = vrpnpos[2];
	//printf("vrpnpos is x: %f ----  y:%f ----- z:%f----------\n", vrpnpos[0], vrpnpos[1], vrpnpos[2]);
#endif
	vec3f_set(pos, 0,0,0);
}


// Calculate the position (in world coordinates) to place the target given the target's distance.
void get_target_position(float pos[3], float target_distance)
{
	pos[0] = start_pos[0] + target_distance * target_vector[0];
	pos[1] = 0.0;
	pos[2] = start_pos[2] + target_distance * target_vector[2];
}


void worldToExpCoord(float output[3], float worldCoord[3])
{
	// Convert to vec4, transform by matrix, convert back to vec3.
	float pos4[4] = { 0,0,0,1 };
	vec3f_copy(pos4,worldCoord);
	mat4f_mult_vec4f(pos4, worldToExpMat);
	vec3f_copy(output, pos4);
}


//record the walked distances in a txt file
void record_trial_distance()
{
	// Get our current position:
	float current_pos[3] = { 0, 0, 0 };
	//get_vrpn_pos_current(current_pos);
	viewmat_get_pos(current_pos, VIEWMAT_EYE_MIDDLE);

	// Calculate distance walked (ignoring height)
	float dist_walked = sqrtf(powf(start_pos[0]-current_pos[0], 2) +
	                          powf(start_pos[2]-current_pos[2], 2));
	float dist_percentage = 100 * dist_walked / trials[current_trial].distance;

	// Print trial/target information
	fprintf(output_file, "%d, %d, %d, %0.1f, ", subjectID, current_trial, trials[current_trial].isPractice, trials[current_trial].distance);
	// Print distance walk as we calculated it:
	fprintf(output_file, "%0.3f, %0.3f, ", dist_walked, dist_percentage);
	// Print starting and current positions (in case we wish to recalculate distance walked differently---i.e., ignore left/right translation):
	fprintf(output_file, "%0.3f, %0.3f, %0.3f, ", actual_start_pos[0],   actual_start_pos[1],   actual_start_pos[2]);
	fprintf(output_file, "%0.3f, %0.3f, %0.3f, ", current_pos[0], current_pos[1], current_pos[2]);

	// Print starting and current positions in experiment coordinates
	float actual_start_pos_expcoord[3];
	worldToExpCoord(actual_start_pos_expcoord, actual_start_pos);
	float current_pos_expcoord[3];
	worldToExpCoord(current_pos_expcoord, current_pos);
	fprintf(output_file, "%0.3f, %0.3f, %0.3f, ", actual_start_pos_expcoord[0], actual_start_pos_expcoord[1], actual_start_pos_expcoord[2]);
	fprintf(output_file, "%0.3f, %0.3f, %0.3f, ", current_pos_expcoord[0], current_pos_expcoord[1], current_pos_expcoord[2]);

    // Print time
	fprintf(output_file, "%ld", time(NULL));
	fprintf(output_file, "\n");

	// Make sure data is written in event of crash.
	fflush(output_file);

	printf("Recorded distance walked for trial %d at time %ld\n", current_trial, time(NULL));
	printf("Distance walked: %0.3f\n", dist_walked);
	printf("Percentage walked: %0.3f\n", dist_percentage);
	printf("\n");
}





void print_current_trial_info()
{
	printf("Ideal starting position (world coord): %0.2f %0.2f %0.2f\n", start_pos[0], start_pos[1], start_pos[2]);

	float current_pos[3] = { 0, 0, 0 };

	//get_vrpn_pos_current(current_pos);
	viewmat_get_pos(current_pos, VIEWMAT_EYE_MIDDLE);
	printf("Current position (world coord): %0.2f %0.2f %0.2f\n", current_pos[0], current_pos[1], current_pos[2]);

	
	// TODO: We are saving start position in a print function?!
	vec3f_copy(actual_start_pos, current_pos);
	float actual_start_pos_expcoord[3];
	worldToExpCoord(actual_start_pos_expcoord, current_pos);
	printf("Current position (exp coord): %0.2f %0.2f %0.2f\n",
	       actual_start_pos_expcoord[0],
	       actual_start_pos_expcoord[1],
	       actual_start_pos_expcoord[2]);
	
	// Get distance from starting position, ignore height:
	float tmp[3];
	vec3f_copy(tmp, actual_start_pos_expcoord);
	tmp[1]=0;
	printf("XZ distance to starting position: %0.2f\n", vec3f_norm(tmp));

	printf("Current trial: %d\n", current_trial);
	printf("Target distance: %0.2f\n", trials[current_trial].distance);
	printf("Target is practice: %d\n", trials[current_trial].isPractice);
	printf("Pointer to target is: %p\n", (void*) trials[current_trial].target);
	printf("Room offset: %f\n", trials[current_trial].roomOffset);
	
	float pos[3];
	get_target_position(pos, trials[current_trial].distance);
	printf("Target position (world coord): %0.2f %0.2f %0.2f\n", pos[0], pos[1], pos[2]);
	float pos_exp[3];
	worldToExpCoord(pos_exp, pos);
	printf("Target position (exp coord): %0.2f %0.2f %0.2f\n", pos_exp[0], pos_exp[1], pos_exp[2]); // verify everything looks correct.
	
	printf("\n");
}

void state_transition(void)
{
	switch (state)
	{
		case EXP_START: // transition from empty room to showing target
			state=EXP_SHOW_TARGET;
			showing_target = 1;
			current_trial = 0;
			print_current_trial_info();
			// TODO: Initialize start_pos ??
			printf("Walk to the target when you are ready.\n\n");		
			break;
		case EXP_SHOW_TARGET: // transition from showing the target to walking (with blank screen)
			state=EXP_WALK;
			blank_screen = 1;
			printf("Currently walking to target.\n\n");
			break;
		case EXP_WALK: // record the distance walked, begin walking back.
			state=EXP_RECORDED;
			record_trial_distance();
			printf("Recorded distance.\n\n");
			break;
		case EXP_RECORDED: // Show the scene for the next trial (or switch to FINISHED state if done).
			
			blank_screen = 0;
			if(current_trial+1 == num_trials)
			{
				state=EXP_FINISHED;
				showing_target = 0;
				// We could fclose() the file, but we don't because the user could go backward through the experiment.
				fflush(output_file);
				printf("Finished!!!\n\n");
			}
			else
			{
				state=EXP_SHOW_TARGET;
				current_trial++;
				print_current_trial_info();
			}
			break;
		case EXP_FINISHED:
			break;
	}
}

void state_transition_backward(void)
{
	switch(state)
	{
		case EXP_START:
			// No change necessary to go back, stay in same state.
			break;
		case EXP_SHOW_TARGET:
			// If we are on first trial, go back to the experiment start state.
			if(current_trial == 0)
			{
				state=EXP_START;
				showing_target = 0;
			}
			else
			{
				// If not on first trial, go back a trial and blank
				// the screen (as if the person is waiting at the end
				// point).
				state=EXP_RECORDED;
				current_trial--;
				blank_screen = 1;
			}
			break;
		case EXP_WALK:
			// Go back to showing the target without a blank screen.
			state=EXP_SHOW_TARGET;
			blank_screen = 0;
			break;
		case EXP_RECORDED:
			// If we have already recorded the data, we need to go
			// back to the walking state (and data will be recorded
			// again when we go back into the recorded state).
			state = EXP_WALK;
			break;
		case EXP_FINISHED:
			// Go back to the state where the participant finished
			// walking on the last trial.
			state = EXP_RECORDED;
			current_trial = num_trials-1; // current_trial is 0 indexed.
			blank_screen = 1;
			showing_target = 1;
			break;
	}
}

/* Called by GLFW whenever a key is pressed. */
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	if (action != GLFW_PRESS)
		return;

	switch (key)
	{
		case GLFW_KEY_W:
			posOffset[0] = posOffset[0] + 0.3;
			printf("Current posOffset:");
			vec3f_print(posOffset);
			break;
		case GLFW_KEY_S:
			posOffset[0] = posOffset[0] - 0.3;
			printf("Current posOffset:");
			vec3f_print(posOffset);
			break;
		case GLFW_KEY_D:
			posOffset[2] = posOffset[2] + 0.3;
			printf("Current posOffset:");
			vec3f_print(posOffset);
			break;
		case GLFW_KEY_A:
			posOffset[2] = posOffset[2] - 0.3;
			printf("Current posOffset:");
			vec3f_print(posOffset);
			break;
		case GLFW_KEY_Q:
			rotOffset++;
			printf("Current rotOffset: %f\n", rotOffset);
			break;
		case GLFW_KEY_E:
			rotOffset--;
			printf("Current rotOffset: %f\n", rotOffset);
			break;
			
		case GLFW_KEY_SPACE:
		case GLFW_KEY_PAGE_DOWN: // clicker: next slide
			state_transition();
			break;

		case GLFW_KEY_B:
		case GLFW_KEY_PAGE_UP: // clicker: previous slide
			state_transition_backward();
			break;

		case GLFW_KEY_I: // print information about camera
		{
			float pos[3];
			viewmat_get_pos(pos, VIEWMAT_EYE_MIDDLE);
			printf("Camera position in world coord (middle): ");
			vec3f_print(pos);
			viewmat_get_pos(pos, VIEWMAT_EYE_LEFT);
			printf("Camera position in world coord (left)  : ");
			vec3f_print(pos);
			viewmat_get_pos(pos, VIEWMAT_EYE_RIGHT);
			printf("Camera position in world coord (right) : ");
			vec3f_print(pos);

			viewmat_get_pos(pos, VIEWMAT_EYE_MIDDLE);
			float expCoord[3];
			worldToExpCoord(expCoord, pos);
			printf("Camera position in experiment coord (middle) : ");
			vec3f_print(expCoord);
			
			printf("\n");
			break;
		}

		case GLFW_KEY_BACKSPACE: // debug mode, show targets
			debug_mode = 1;
			break;
	}
}


void draw_target(float viewMat[16], float perspective[16], int trialNum)
{
	/* Draw the target model */
	float targ_position[3];
	get_target_position(targ_position, trials[trialNum].distance);
	float scaleMat[16];
	mat4f_scale_new(scaleMat, .5, .5, .5);
	float modelMat[16], modelview[16];
	mat4f_translate_new(modelMat, targ_position[0], targ_position[1], targ_position[2]);
	// modelview = viewMat * modelMat * scaleMat
	mat4f_mult_mat4f_many(modelview, viewMat, modelMat, scaleMat, NULL);
	
	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0,
	                   perspective);
	glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, // number of 4x4 float matrices
	                   0, // transpose
	                   modelview); // value
	kuhl_geometry_draw(trials[trialNum].target);
}
	


/** Called by GLUT whenever the window needs to be redrawn. This
* function should not be called directly by the programmer. Instead,
* we can call glutPostRedisplay() to request that GLUT call display()
* at some point. */
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
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f); // set clear color to black
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

		//Rotate the room based on the hardcoded room offset angle.
		float offsetRot[16];
		mat4f_rotateAxis_new(offsetRot, rotOffset, 0, 1, 0);

		//Translate the room based on the hardcoded room offset position.
		float offsetPos[16];
		mat4f_translate_new(offsetPos, posOffset[0], posOffset[1], posOffset[2]);

		float trialOffset[16];
		mat4f_translate_new(trialOffset,
		                    target_vector[0]*trials[current_trial].roomOffset,
		                    target_vector[1]*trials[current_trial].roomOffset,
		                    target_vector[2]*trials[current_trial].roomOffset);

		
		float modelview[16];
		mat4f_mult_mat4f_many(modelview, viewMat, trialOffset, offsetRot, offsetPos, NULL);

		/* Send the modelview matrix to the vertex program. */
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
			1, // number of 4x4 float matrices
			0, // transpose
			modelview); // value

		glUniform1i(kuhl_get_uniform("renderStyle"), 1); // use color from texture only; no diffuse shading.

		kuhl_errorcheck();

		/* Draw the model */
		kuhl_geometry_draw(modelgeom);
		kuhl_errorcheck();


		if(debug_mode)
		{
			/* Draw all the targets */
			for(int i=0; i<num_trials; i++)
				draw_target(viewMat, perspective, i);

			float transMat[16],modelview[16];
			mat4f_translateVec_new(transMat, start_pos);
			mat4f_mult_mat4f_new(modelview, viewMat, transMat);
			glUniformMatrix4fv(kuhl_get_uniform("ModelView"),1,0,modelview);
			glUniformMatrix4fv(kuhl_get_uniform("Projection"),1,0,perspective);
			kuhl_geometry_draw(origingeom);
		}
		else if(showing_target)
			draw_target(viewMat, perspective, current_trial);

		if(blank_screen)
		{
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // set clear color to black
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		}


		glUseProgram(0); // stop using a GLSL program.
		viewmat_end_eye(viewportID);
	} // finish viewport loop

	  /* Update the model for the next frame based on the time. We
	  * convert the time to seconds and then use mod to cause the
	  * animation to repeat. */
	double time = glfwGetTime();
	dgr_setget("time", &time, sizeof(double));
	kuhl_update_model(modelgeom, 0, fmodf((float)time, 10.0f));

	viewmat_end_frame();

	/* Check for errors. If there are errors, consider adding more
	* calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

	//kuhl_video_record("videoout", 30);
}

/* Initializes our list of trials. */
void init_trials()
{
	#define num_modeltargets 5
	kuhl_geometry *modeltargets[num_modeltargets]; // target models


	/* Create a matrix which converts from world coordinates into
	 * "experiment" coordinates. In "experiment" coordinates, the
	 * origin is at the starting position and the targets are placed
	 * on the -Z axis. The Y axis is the user's height. The X axis
	 * points to the right. */
	vec3f_normalize(target_vector);
	// negate target vector because this vector is actually pointing down -Z.
	float target4[4] = { -target_vector[0], -target_vector[1], -target_vector[2], 0 };
	float up[4] = { 0, 1, 0, 0};
	float right[4] = { 0,0,0,0};
	vec3f_cross_new(right, target_vector, up);
	// Create a matrix to translate the starting position to the origin.
	float transMat[16];
	mat4f_translate_new(transMat, -start_pos[0], -start_pos[1], -start_pos[2]);
	float rotMat[16];
	mat4f_identity(rotMat);
	mat4f_setRow(rotMat, right, 0);
	mat4f_setRow(rotMat, up, 1);
	mat4f_setRow(rotMat, target4, 2);
	// Translate the starting position to the origin then rotate appropriately.
	mat4f_mult_mat4f_new(worldToExpMat, rotMat, transMat);


	
	
	// All of the target models.
	// On the Rekhi lab machines, these files are stored in /local/kuhl-public-share/opengl/data/...
	modeltargets[0] = kuhl_load_model("models/targets-bochao/cylinder_green_s.dae", NULL, program, NULL);
	modeltargets[1] = kuhl_load_model("models/targets-bochao/sq_yellow.dae", NULL, program, NULL);
	modeltargets[2] = kuhl_load_model("models/targets-bochao/cross_red.dae", NULL, program, NULL);
	modeltargets[3] = kuhl_load_model("models/targets-bochao/poly_brown.dae", NULL, program, NULL);
	modeltargets[4] = kuhl_load_model("models/targets-bochao/trian_blue_s.dae", NULL, program, NULL);

	
	const int num_exp_trials = 15;
	float exp_trials[15] = { 2, 2, 2, 2.5, 3, 3, 3, 3.5, 4, 4, 4, 4.5, 5, 5, 5 };
	kuhl_shuffle(exp_trials, num_exp_trials, sizeof(float));

	float offsets[15] = { 0.5, 0.5, 0.5, 0.5, 0.5,
	                      0, 0, 0, 0, 0,
	                      -0.5, -0.5, -0.5, -0.5, -0.5 };
	kuhl_shuffle(offsets, num_exp_trials, sizeof(float));
	
	const int num_practice_trials = 2;
	float practice_trials[2] = { 3.5, 5.5 };
	kuhl_shuffle(practice_trials, num_practice_trials, sizeof(float));
	
	num_trials = num_exp_trials+num_practice_trials;

	// Allocate and zero out trials array.
	trials = malloc(num_trials * sizeof(trial));
	for(int i=0; i<num_trials; i++)
	{
		trials[i].distance = 0;
		trials[i].isPractice = 0;
		trials[i].target = NULL;
		trials[i].roomOffset = 0;
	}
	
    /* Copy practice trials and actual trials into a single array. */
	int i=0;
	for(; i<num_practice_trials; i++)
	{
		trials[i].distance = practice_trials[i];
		trials[i].isPractice = 1;
	}
	for(; i<num_trials; i++)
	{
		trials[i].distance = exp_trials[i-num_practice_trials];
		// practice / throwaway data if distance isn't a whole number:
		if(trials[i].distance - (int)trials[i].distance > .1)
			trials[i].isPractice=1;
	}

	// Copy room offsets only for the real trials
	for(int i=num_practice_trials; i<num_trials; i++)
		trials[i].roomOffset = offsets[i-num_practice_trials];

	/* Select random targets */
	for(int i=0; i<num_trials; i++)
	{
		int targetindex = kuhl_randomInt(0, num_modeltargets-1);
		trials[i].target = modeltargets[targetindex];
	}
}

int main(int argc, char** argv)
{
	srand(time(NULL));

	printf("Enter subject ID number for data file: ");
	fflush(stdout);

	char input[1024];
	if(fgets(input, 1024, stdin) == NULL)
	{
		msg(MSG_FATAL, "fgets failed");
		exit(1);
	}
	int ret = sscanf(input, "%d", &subjectID);
	while(ret != 1)
	{
		printf("ERROR: Enter a valid number.\n");
		if(fgets(input, 1024, stdin) == NULL)
		{
			msg(MSG_FATAL, "fgets failed");
			exit(1);
		}
		ret = sscanf(input, "%d", &subjectID);
	}
	
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 960, 540, 32, 4);

	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), keyboard);
	// glfwSetFramebufferSizeCallback(window, reshape);

	/* Compile and link a GLSL program composed of a vertex shader and
	* a fragment shader. */
	program = kuhl_create_program("viewer.vert", "viewer.frag");

	dgr_init();     /* Initialize DGR based on environment variables. */


	/** Initial position of the camera. 1.55 is a good approximate
	 * eyeheight in meters. */
	const float initCamPos[3] = { start_pos[0],1.55f,start_pos[2] };
	
    /** A point that the camera should initially be looking at. If
     * fitToView is set, this will also be the position that model will be
     * translated to.
     *
     * We will look at a point on the floor in the direction of the
     * target 3 meters away by default.
     */
	const float initCamLook[3] = { start_pos[0]+target_vector[0]*3,
	                               0.0f,
	                               start_pos[2]+target_vector[2]*3 };
	
	/** A vector indicating which direction is up. */
	const float initCamUp[3] = { 0.0f,1.0f,0.0f };
	
	viewmat_init(initCamPos, initCamLook, initCamUp);

	// Clear the screen while things might be loading
	glClearColor(.2f, .2f, .2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Load the model from the file
	modelgeom = kuhl_load_model("models/lab-mtu/lab_minification-floorfixed.dae", NULL, program, NULL);
	origingeom = kuhl_load_model("models/origin/origin.obj", NULL, program, NULL);

	init_trials();
	
	// for experiment recording
#if defined _WIN32
	CreateDirectory("./distjudge-results", NULL);
#else
	mkdir("./distjudge-results", 0700);
#endif
	const char *fname = "./distjudge-results/dist.csv";
	output_file = fopen(fname, "a");
	if (!output_file) {
		msg(MSG_FATAL, "Unable to open %s for writing.\n", fname);
		exit(EXIT_FAILURE);
	}
	fprintf(output_file, "\n\n");
	//subject ID
	time_t mytime = time(NULL);
	fprintf(output_file, "# Time: %s", ctime(&mytime));
	fprintf(output_file, "SubjID,Trial,Practice,Target_Distance,Walked_Distance,Walked_Percent,ActualStartXWorld,ActualStartYWorld,ActualStartZWorld,EndPosXWorld,EndPosYWorld,EndPosZWorld,ActualStartXExp,ActualStartYExp,ActualStartZExp,EndPosXExp,EndPosYExp,EndPosZExp,Time\n");
	fflush(output_file);


	// Some models use two triangles for walls, etc. so that one
	// triangle is to be seen from outside the room and another
	// triangle is used for the walls to be seen from the inside of
	// the room. Here, we don't draw triangles facing away from us.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_CLAMP);
	glDepthFunc(GL_LEQUAL); // eliminates near plane clipping


	while(!glfwWindowShouldClose(kuhl_get_window()))
	{
		display();
		kuhl_errorcheck();

		/* process events (keyboard, mouse, etc) */
		glfwPollEvents();
	}
	exit(EXIT_SUCCESS);
}

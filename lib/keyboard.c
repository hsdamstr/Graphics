
#include "libkuhl.h"
#include "keyboard.h"
#include <GLFW/glfw3.h>

/* Called by GLFW whenever a key is pressed. */
bool kuhl_keyboard_handler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return false;

	// escape button closes the window
	if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		return true;
	}

	// the following keys require shift modifier
	if (!(mods & GLFW_MOD_SHIFT))
		return false;

	switch (key)
	{
	case GLFW_KEY_Q:
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;

	case GLFW_KEY_S:
		kuhl_screenshot("screenshot.png");
		break;
			
	case GLFW_KEY_F: // toggle full screen
	{
		GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *currentMode = glfwGetVideoMode(primaryMonitor);
		if(glfwGetWindowMonitor(kuhl_get_window())) // if full screen
			// make windowed:
			glfwSetWindowMonitor(kuhl_get_window(), NULL, 0, 0, 512, 512, GLFW_DONT_CARE);
		else // if windowed
			// make full screen:
			glfwSetWindowMonitor(kuhl_get_window(), primaryMonitor, 0,0,currentMode->width, currentMode->height, currentMode->refreshRate);
		break;
	}
	case GLFW_KEY_W:
	{
		// Toggle between wireframe and solid
		GLint polygonMode[2];
		glGetIntegerv(GL_POLYGON_MODE, polygonMode);
		if (polygonMode[0] == GL_LINE)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	}
	case GLFW_KEY_P:
	{
		// Toggle between points and solid
		GLint polygonMode[2];
		glGetIntegerv(GL_POLYGON_MODE, polygonMode);
		if (polygonMode[0] == GL_POINT)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	}
	case GLFW_KEY_C:
	{
		// Toggle front, back, and no culling
		GLint cullMode;
		glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
		if (glIsEnabled(GL_CULL_FACE))
		{
			if (cullMode == GL_FRONT)
			{
				glCullFace(GL_BACK);
				printf("Culling: Culling back faces; drawing front faces\n");
			}
			else
			{
				glDisable(GL_CULL_FACE);
				printf("Culling: No culling; drawing all faces.\n");
			}
		}
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			printf("Culling: Culling front faces; drawing back faces\n");
		}
		kuhl_errorcheck();
		break;
	}
	case GLFW_KEY_D: // toggle depth clamping
	{
		if (glIsEnabled(GL_DEPTH_CLAMP))
		{
			printf("Depth clamping disabled\n");
			glDisable(GL_DEPTH_CLAMP); // default
			glDepthFunc(GL_LESS);      // default
		}
		else
		{
			/* With depth clamping, vertices beyond the near and
			   far planes are clamped to the near and far
			   planes. Since multiple layers of vertices will be
			   clamped to the same depth value, depth testing
			   beyond the near and far planes won't work. */
			printf("Depth clamping enabled\n");
			glEnable(GL_DEPTH_CLAMP);
			glDepthFunc(GL_LEQUAL); // makes far clamping work.
		}
		break;
	}
	case GLFW_KEY_EQUAL:  // The = and + key on most keyboards
	case GLFW_KEY_KP_ADD: // increase size of points and width of lines
	{
		// How can we distinguish between '=' and '+'? The 'mods'
		// variable should contain GLFW_MOD_SHIFT if the shift key
		// is pressed along with the '=' key. However, we accept
		// both versions.

		GLfloat currentPtSize;
		GLfloat sizeRange[2] = { -1.0f, -1.0f };
		glGetFloatv(GL_POINT_SIZE, &currentPtSize);
		glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, sizeRange);
		GLfloat temp = currentPtSize + 1;
		if (temp > sizeRange[1])
			temp = sizeRange[1];
		glPointSize(temp);
		printf("Point size is %f (can be between %f and %f)\n", temp, sizeRange[0], sizeRange[1]);
		kuhl_errorcheck();

		// The only line width guaranteed to be available is
		// 1. Larger sizes will be available if your OpenGL
		// implementation or graphics card supports it.
		GLfloat currentLineWidth;
		GLfloat widthRange[2] = { -1.0f, -1.0f };
		glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);
		glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, widthRange);
		temp = currentLineWidth + 1;
		if (temp > widthRange[1])
			temp = widthRange[1];
		glLineWidth(temp);
		printf("Line width is %f (can be between %f and %f)\n", temp, widthRange[0], widthRange[1]);
		kuhl_errorcheck();
		break;
	}
	case GLFW_KEY_MINUS: // decrease size of points and width of lines
	case GLFW_KEY_KP_SUBTRACT:
	{
		GLfloat currentPtSize;
		GLfloat sizeRange[2] = { -1.0f, -1.0f };
		glGetFloatv(GL_POINT_SIZE, &currentPtSize);
		glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, sizeRange);
		GLfloat temp = currentPtSize - 1;
		if (temp < sizeRange[0])
			temp = sizeRange[0];
		glPointSize(temp);
		printf("Point size is %f (can be between %f and %f)\n", temp, sizeRange[0], sizeRange[1]);
		kuhl_errorcheck();

		GLfloat currentLineWidth;
		GLfloat widthRange[2] = { -1.0f, -1.0f };
		glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);
		glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, widthRange);
		temp = currentLineWidth - 1;
		if (temp < widthRange[0])
			temp = widthRange[0];
		glLineWidth(temp);
		printf("Line width is %f (can be between %f and %f)\n", temp, widthRange[0], widthRange[1]);
		kuhl_errorcheck();
		break;
	}
	default:
		return false;
	}
	return true;
}

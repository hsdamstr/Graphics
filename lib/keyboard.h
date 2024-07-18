#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <GLFW/glfw3.h>
#include <stdbool.h>

bool kuhl_keyboard_handler(GLFWwindow* window, int key, int scancode, int action, int mods);

#ifdef __cplusplus
} // end extern "C"
#endif

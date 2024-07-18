#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position; // vertex position, object coordinates

uniform mat4 ModelView;
uniform mat4 Projection;

uniform int red;

void main() 
{
	// Append a 1 to the end of our position in camera coordinates to
	// get a vec4.
	vec4 pos = vec4(in_Position, 1.0);

	// Calculate the position of the vertex in NDC.
	gl_Position = Projection * ModelView * pos;
}

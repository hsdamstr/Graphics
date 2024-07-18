#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position; // vertex position, object coordinates
in vec3 in_Color;    // vertex color

uniform mat4 ModelView;
uniform mat4 Projection;

out vec3 color;

void main() 
{
	// Output the color into the fragment program
	color = in_Color;

	// Calculate the position of the vertex in NDC.
	vec4 pos = vec4(in_Position, 1.0);
	gl_Position = Projection * ModelView * pos;
}

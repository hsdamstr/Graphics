#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec2 out_TexCoord;

uniform sampler2D tex;

void main() 
{
	if(true)
	{
		fragColor = texture(tex, out_TexCoord);
	}
	else
	{
		// Debugging tip: Try displaying texture coordinate as a
		// color.
		fragColor = vec4(out_TexCoord,0,1);
	}
}

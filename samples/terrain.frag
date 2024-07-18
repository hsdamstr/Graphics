#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec3 out_TexCoord;
in vec3 sNorm;

uniform sampler2D tex1;
uniform int isOn;
in vec4 camCoords;

void main() 
{

	vec3 lightPos = vec3(0,0,0);


	vec3 lightDir = lightPos - camCoords.xzy;

	float diffuse = dot(normalize(lightDir), normalize(sNorm));
	diffuse = abs(diffuse);
	diffuse = clamp(diffuse , .2 , 1);

	if(isOn == 0){
		fragColor = texture(tex1, out_TexCoord.xy);
		fragColor.r =fragColor.r * diffuse;
		fragColor.g =fragColor.g * diffuse;
		fragColor.b =fragColor.b * diffuse;
		fragColor.a = 1;
	}
	
	else
	{
		fragColor = texture(tex1, out_TexCoord.xy);
		fragColor.r = clamp(sNorm.x, .5 , 1);
		fragColor.b = clamp(sNorm.y , .5 , 1);
		fragColor.g = clamp(sNorm.z, .5 , 1);

	}
}

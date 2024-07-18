#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position;
in vec3 in_TexCoord;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform sampler2D tex;

out vec3 out_TexCoord;
out vec3 sNorm;
out vec4 camCoords;

void main() 
{
	
	mat3 normalMat = transpose(inverse(mat3(ModelView)));

	vec4 colorOrig = (texture(tex , in_TexCoord.xy)); //original color 
	vec3 orig = vec3(in_Position.x, in_Position.y, colorOrig.r / 10);
	
	vec2 rightTex = vec2(in_TexCoord.x + .001, in_TexCoord.y);
	vec4 colorRight = (texture(tex , rightTex)); //color a little to the right
	vec3 right = vec3(rightTex.x, in_TexCoord.y , colorRight.r / 10);

	vec2 upTex = vec2(in_TexCoord.x, in_TexCoord.y + .001);
	vec4 colorUp = (texture(tex , upTex)); //color a little up
	vec3 up = vec3(in_TexCoord.x , upTex.y, colorUp.r / 10);

	sNorm = normalize(cross((orig - up), (up-right)) * normalMat);
	if(sNorm.x < 0){
		sNorm.x = sNorm.x * -1;
	}
	if(sNorm.y < 0){
		sNorm.y = sNorm.y * -1;
	}
	if(sNorm.z < 0){
		sNorm.z = sNorm.z * -1;
	}

	vec4 pos = vec4(in_Position.x, in_Position.y, colorOrig.r / 10, 1.0);
	
	camCoords = ModelView * vec4(in_Position, 1);

	out_TexCoord = in_TexCoord;
	gl_Position = Projection * ModelView * pos;
}

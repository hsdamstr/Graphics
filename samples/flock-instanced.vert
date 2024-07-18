#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position; /* Position of vertex (object coordinates) */
in vec2 in_TexCoord; /* Texture coordinate */
in vec3 in_Normal;   /* Normal vector at this vertex (object coordinates) */
in vec3 in_Color;    /* Vertex color */

in vec4 in_BoneIndex;
in vec4 in_BoneWeight;
uniform mat4 BoneMat[128];
uniform int NumBones;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 GeomTransform;

out vec2 out_TexCoord;
out vec3 out_Color;
out vec3 out_Normal_CC;   // normal vector (camera coordinates)
out vec3 out_Position_CC; // vertex position (camera coordinates)

// https://stackoverflow.com/q/12964279
// https://www.wolframalpha.com/input?i=plot%28+mod%28+sin%28x*12.9898+%2B+y*78.233%29+*+43758.5453%2C1%29x%3D0..2%2C+y%3D0..2%29
float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 getRandomPosition()
{
	vec3 pos;
	pos.x = rand(vec2(gl_InstanceID, 0));
	pos.y = rand(vec2(gl_InstanceID, 1));
	pos.z = rand(vec2(gl_InstanceID, 2));

	pos = pos * 50 - 25;
	return vec4(pos, 1);
}

void main()
{
	// Copy texture coordinates and color to fragment program
	out_TexCoord = in_TexCoord;
	out_Color = in_Color;

	// Get random position from instance ID
	mat4 randomPosMat = mat4(1); // identity
	// If instance ID is 1, don't translate. This allows normal draw
	// calls without instances to be drawn without the extra random
	// transformation applied. In flock-instanced, it makes the FPS
	// display correctly.
	if(gl_InstanceID > 0)
		randomPosMat[3] = getRandomPosition();  // set 4th column

	/* Calculate the actual modelview matrix: */
	mat4 actualModelView;
	if(NumBones > 0)
	{
		/* If we have an animated model/character that contains bones,
		   we need to account for the bone matrices. */
		mat4 m = in_BoneWeight.x * BoneMat[int(in_BoneIndex.x)] +
		         in_BoneWeight.y * BoneMat[int(in_BoneIndex.y)] +
		         in_BoneWeight.z * BoneMat[int(in_BoneIndex.z)] +
		         in_BoneWeight.w * BoneMat[int(in_BoneIndex.w)];
		actualModelView = ModelView * randomPosMat * m;
	}
	else
		/* If we have a model without animation/bones in it, we simply
		 * need to account for the GeomTransform matrix embedded in
		 * the 3D model. */
		actualModelView = ModelView * randomPosMat * GeomTransform;


	mat3 NormalMat = transpose(inverse(mat3(actualModelView)));

	// Transform normal from object coordinates to camera coordinates
	out_Normal_CC = normalize(NormalMat * in_Normal);

	// Transform vertex from object to unhomogenized Normalized Device
	// Coordinates (NDC).
	gl_Position = Projection * actualModelView * vec4(in_Position, 1);

	// Calculate the position of the vertex in camera coordinates:
	out_Position_CC = vec3(actualModelView * vec4(in_Position, 1));
}

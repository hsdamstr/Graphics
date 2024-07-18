#version 150 // GLSL 150 = OpenGL 3.2

/* Vertex position and normal vector from the C program. These are in
 * object coordinates. */
in vec3 in_Position;
in vec3 in_Normal;

/* We output a position and normal in "camera coordinates = CC". After
 * the vertex program is run, these are interpolated across the
 * object, so they are no longer the vertex position and normal in the
 * fragment program...they are the fragment's position and normal. */
out vec4 out_Position_CC;
out vec3 out_Normal_CC;

uniform mat4 Projection;
uniform mat4 ModelView;

uniform int red;

void main() 
{
	// Construct a normal matrix from the ModelView matrix. The
	// modelview is the same for all vertices, the normal matrix is
	// too. It would be more efficient to calculate it in our C
	// program once for this object. However, it is easier to
	// calculate here.
	mat3 NormalMat = transpose(inverse(mat3(ModelView)));
	
	// Transform the normal by the NormalMat and send it to the
	// fragment program.
	out_Normal_CC = normalize(NormalMat * in_Normal);

	// Transform the vertex position from object coordinates into
	// camera coordinates.
	out_Position_CC = ModelView * vec4(in_Position, 1);

	// Transform the vertex position from object coordinates into
	// Normalized Device Coordinates (NDC).
	gl_Position = Projection * ModelView * vec4(in_Position, 1);
}

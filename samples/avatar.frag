#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec2 out_TexCoord; // Vertex texture coordinate
in vec3 out_Color;    // Vertex color
in vec3 out_Normal_CC;   // Normal vector in camera coordinates
in vec3 out_Position_CC; // Position of fragment in camera coordinates
in mat3 NormalMat;
in vec3 out_Normal;
in vec3 out_Tangent;
in vec3 out_Bitangent;
uniform sampler2D tex_NORMALS;
uniform sampler2D tex_SPECULAR;
uniform int HasTex;    // Is there a texture in tex?
uniform sampler2D tex; // Diffuse texture
uniform int renderStyle;


/** Calculate diffuse shading. Normal and light direction do not need
 * to be normalized. */
float diffuseScalar(vec3 normal, vec3 lightDir, bool frontBackSame)
{
	/* Basic equation for diffuse shading */
	float diffuse = dot(normalize(lightDir), normalize(normal));

	/* The diffuse value will be negative if the normal is pointing in
	 * the opposite direction of the light. Set diffuse to 0 in this
	 * case. Alternatively, we could take the absolute value to light
	 * the front and back the same way. Either way, diffuse should now
	 * be a value from 0 to 1. */
	if(frontBackSame)
		diffuse = abs(diffuse);
	else
		diffuse = clamp(diffuse, 0, 1);

	/* Keep diffuse value in range from .5 to 1 to prevent any object
	 * from appearing too dark. Not technically part of diffuse
	 * shading---however, you may like the appearance of this. */
	diffuse = diffuse/2 + .5;

	return diffuse;
}

float specular(vec3 normal, vec3 lightDir, vec3 out_Position_CC)
{
	/* Basic equation for diffuse shading */
	vec3 nlightDir = normalize(lightDir);
	vec3 nout_position_CC = normalize(out_Position_CC);

	vec3 sum = nlightDir + nout_position_CC;
	vec3 half_angle = normalize(sum);

	float spec = pow(dot(half_angle, normal), 10);

	return spec;
}


void main() 
{
	/* Get position of light in camera coordinates. When we do
	 * headlight style rendering, the light will be at the position of
	 * the camera! */
	vec3 lightPos = vec3(0,1000,0);

	vec4 colorFromTex = texture(tex, out_TexCoord);

	/* Calculate a vector pointing from our current position (in
	 * camera coordinates) to the light position. */
	vec3 lightDir = lightPos - out_Position_CC;

	/* Calculate diffuse shading */
	float diffuse = diffuseScalar(out_Normal_CC, lightDir, true);

	mat3 TBN = mat3(out_Tangent, out_Bitangent, out_Normal);


	vec4 color = texture(tex_NORMALS, out_TexCoord);
	color.r = (pow(color.r, 1/2.2) * 2) - 1;
	color.g = (pow(color.g, 1/2.2) * 2) - 1;
	color.b = (pow(color.b, 1/2.2) * 2) - 1;

	vec4 how_much_specular = texture(tex_SPECULAR, out_TexCoord);
	how_much_specular.r = pow(how_much_specular.r, 1/2.2);
	

	vec3 adjustedNormal = normalize(NormalMat * TBN * color.rgb);

	float original_diffuse = diffuseScalar(out_Normal_CC, lightDir, true);
	float adj_diffuse = diffuseScalar(adjustedNormal, lightDir, true);

	float adjusted_specular = how_much_specular.r * specular(adjustedNormal, lightDir, out_Normal_CC);
	
	if(renderStyle == 0)
	{
		fragColor = vec4(diffuse,diffuse,diffuse, 1);
	}
	else if(renderStyle == 4)
	{
		fragColor = texture(tex, out_TexCoord);
	
	}
	else if(renderStyle == 5)
	{
		fragColor = texture(tex, out_TexCoord);

		fragColor.xyz = fragColor.xyz * adj_diffuse;
	}
	else if(renderStyle == 3)
	{
		adjustedNormal.xyz = (adjustedNormal.xyz + 1)/2;
		fragColor = vec4(adjustedNormal, 1);
	}
	else if(renderStyle == 1)
	{
		fragColor = vec4(adj_diffuse, adj_diffuse, adj_diffuse , 1);
	}
	else if(renderStyle == 2)
	{
		fragColor.xyz = (normalize(out_Normal_CC) + 1)/2;
		fragColor.a = 1;
	}
	else if(renderStyle == 6)
	{
		fragColor.rgb = (colorFromTex.rgb * adj_diffuse) + (adjusted_specular * vec3(1,1,1));

	}
}

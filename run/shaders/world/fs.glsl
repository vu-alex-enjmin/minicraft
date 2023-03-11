#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec4 color;
in vec2 uv;
in float type;
in float ao;

out vec4 color_out;

void main()
{
	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	vec3 litColor = color.xyz * fragAo;
	color_out = vec4(litColor, color.a);
}
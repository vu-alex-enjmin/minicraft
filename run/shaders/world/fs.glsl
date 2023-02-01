#version 400

//Variables en entree
in vec3 normal;
in vec4 color;
in vec2 uv;

out vec4 color_out;

//Globales
const float ambientLevel = 0.4;

void main()
{
	vec3 toLight = normalize(vec3(0,1,1));
	color_out = vec4(sqrt(color.xyz * max(0,dot(toLight,normal)) * 0.97 + 0.03 * vec3(0.8,0.9,1)),color.a);
}
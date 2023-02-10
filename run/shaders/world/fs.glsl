#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec4 color;
in vec2 uv;
in float type;
in float ao;

uniform mat4 m;

uniform vec3 camera_pos;
uniform vec3 sun_color;
uniform vec3 ambient_color;
uniform vec3 sun_direction;

out vec4 color_out;

//Globales
// const float ambientLevel = 0.4;

#define CUBE_EAU 4.0

void main()
{
	vec3 viewVec = normalize(camera_pos - worldPos.xyz);
	vec3 sunDir = normalize(sun_direction);
	vec3 sunHalfVec = normalize(viewVec + sunDir); 

	float sunLightDiffuse = max(0, dot(sunDir, normal));

	float sunLightSpecular;
	if (sunLightDiffuse > 0)
		sunLightSpecular = max(0, dot(sunHalfVec, normal));
	else
		sunLightSpecular = 0;

	if (type == CUBE_EAU)
		sunLightSpecular = pow(sunLightSpecular, 1000);
	else
		sunLightSpecular = pow(sunLightSpecular, 3) * 0.4;

	float ambientAmount = max(0.8, dot(normalize(vec3(-0.25, -0.5, 1.0)), normal)) * 0.5;

	vec3 baseColor = color.xyz;
	vec3 diffuseColor = baseColor * ((sunLightDiffuse * sun_color) + (ambient_color * ambientAmount));
	vec3 specularColor = sunLightSpecular * sun_color;

	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	vec3 litColor = diffuseColor * fragAo + specularColor;
	
	color_out = vec4(litColor, color.a);
}
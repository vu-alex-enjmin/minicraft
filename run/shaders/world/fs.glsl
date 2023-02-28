#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec4 color;
in vec2 uv;
in float type;
in float ao;
in float normalizedDistToCamera;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

uniform float elapsed;
uniform vec3 camera_pos;
uniform vec3 sun_color;
uniform vec3 ambient_color;
uniform vec3 fog_color;
uniform vec3 sun_direction;

out vec4 color_out;

#define CUBE_EAU 4.0

float noise(vec2 pos2D)
{
	vec2 firstNoiseOffset = vec2(+123.21, +413.71);
	float firstValue = sin(length(pos2D + firstNoiseOffset) * 0.5 - elapsed * 3);

	vec2 secondNoiseOffset = vec2(+80.21, +500.71);
	float secondValue = sin(length(pos2D + secondNoiseOffset) * 0.5 - elapsed * 3);

	return (((firstValue + secondValue) * 0.5) * 0.5 + 0.5);
}

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

	if (abs(type-CUBE_EAU) < 1e-6)
		sunLightSpecular = pow(sunLightSpecular, 500);
	else
		sunLightSpecular = pow(sunLightSpecular, 2) * 0.33;

	float ambientAmount = max(0.8, dot(normalize(vec3(-0.25, -0.5, 1.0)), normal)) * 0.5;

	vec3 baseColor = color.xyz;
	vec3 diffuse = baseColor * ((sunLightDiffuse) * sun_color);
	vec3 ambient = baseColor * (ambientAmount * ambient_color);
	vec3 specular = (sunLightSpecular) * sun_color;

	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	// fragAo = 1.0;
	vec3 litColor = (ambient + diffuse + specular) * fragAo;
	// litColor = fragAo * (baseColor * 0.25 + baseColor * ambient_color * 0.75);
	// color_out = vec4(mix(litColor, fog_color, normalizedDistToCamera), color.a);
	color_out = vec4(litColor, color.a);
	// color_out = vec4(fragAo, fragAo, fragAo, color.a);

	if (type == CUBE_EAU)
	{
		float noiseValue = noise(worldPos.xy);
		// color_out = vec4(noiseValue, noiseValue, noiseValue, 1);
	}
}
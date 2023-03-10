#version 400

uniform float elapsed;
uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
uniform mat4 mvp;
uniform mat4 nmat;
uniform vec3 camera_pos;

uniform float fog_density;
uniform float fog_min_distance;

layout(location=0) in vec3 vs_position_in;
layout(location=1) in vec3 vs_normal_in;
layout(location=2) in vec2 vs_uv_in;
layout(location=3) in float vs_type_in;
layout(location=4) in float vs_ao_in;

//Variables en sortie
out vec4 worldPos;
out vec3 normal;
out vec4 color;
out vec2 uv;
out float ao;
out float fogFactor;
out float actualClipZ;

#define WORLD_SIZE (64.0 * 4.0)

float getNoise(vec2 pos)
{
	float time = elapsed;
	vec2 source1 = vec2(237.511, 27.421);
	vec2 source2 = vec2(-137.456, 97.7411);
	vec2 source3 = vec2(89.723, 284.342);
	vec2 source4 = vec2(81.3477, -81.4215);

	float noiseValue = 0;
	noiseValue += sin(length(pos - source1) * 3.23 - 1.87 * time) * 0.1;
	noiseValue += sin(length(pos - source2) * 1.05 - 1.08 * time) * 0.4;
	noiseValue += sin(length(pos - source3) * 2.27 - 2.37 * time) * 0.1;
	noiseValue += sin(length(pos - source4) * 1.53 - 2.17 * time) * 0.4;

	noiseValue = noiseValue * 0.05 - 0.125;

	return noiseValue;
}

void main()
{
	vec4 vecIn = vec4(vs_position_in, 1.0);
	worldPos = m * vecIn;
	worldPos.z += getNoise(worldPos.xy);
	
	gl_Position = (p * v) * worldPos;
	actualClipZ = gl_Position.z;
	
	normal = (nmat * vec4(vs_normal_in, 1.0)).xyz; 

	uv = vs_uv_in;
	ao = vs_ao_in;

	color = vec4(0.1, 0.5, 0.7, 0.45);

	fogFactor = max(0, length(worldPos.xyz - camera_pos) - fog_min_distance);
	fogFactor = exp2(-fogFactor * fog_density * fogFactor * fog_density);
	fogFactor = 1 - fogFactor;
}

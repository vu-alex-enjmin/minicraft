#version 400

uniform float elapsed;
uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
uniform mat4 mvp;
uniform mat4 nmat;
uniform vec3 camera_pos;

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
out float normalizedDistToCamera;
out float actualClipZ;

#define WORLD_SIZE (64.0 * 4.0)

float getNoise(vec2 pos)
{
	float time = elapsed;
	vec2 source1 = vec2(-127.511, 153.421);
	vec2 source2 = vec2(-90.456, -30.7411);
	vec2 source3 = vec2(325.723, 80.342);
	vec2 source4 = vec2(20.3477, -60.4215);
	float noiseValue = 0;
	noiseValue = sin(length(pos - source1) * 3.27 - 1.25 * time) * 0.4;
	noiseValue += sin(length(pos - source2) * 2.38 - 1.02 * time) * 0.4;
	noiseValue += sin(length(pos - source3) * 7.23 - 2.14 * time) * 0.2;
	noiseValue += sin(length(pos - source4) * 5.24 - 3.57 * time) * 0.2;
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

	normalizedDistToCamera = clamp(length(worldPos.xyz - camera_pos) / (WORLD_SIZE * 0.5), 0.0, 1.0);
	normalizedDistToCamera = normalizedDistToCamera * normalizedDistToCamera;
}

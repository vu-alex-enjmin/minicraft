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
out vec2 uv;
out float ao;
out float fogFactor;
out float actualClipZ;

void main()
{
	vec4 vecIn = vec4(vs_position_in, 1.0);
	worldPos = m * vecIn;
	
	gl_Position = (p * v) * worldPos;
	actualClipZ = gl_Position.z;
	
	normal = (nmat * vec4(vs_normal_in, 1.0)).xyz; 

	uv = vs_uv_in;
	ao = vs_ao_in;

	fogFactor = max(0, length(worldPos.xyz - camera_pos) - fog_min_distance);
	fogFactor = exp2(-fogFactor * fog_density * fogFactor * fog_density);
	fogFactor = 1 - fogFactor;
}
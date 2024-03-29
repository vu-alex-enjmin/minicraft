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
out float type;
out float ao;

#define CUBE_HERBE 0.0
#define CUBE_TERRE 1.0
#define CUBE_PIERRE 3.0
#define CUBE_EAU 4.0
#define CUBE_SABLE_01 17.0
#define CUBE_BRANCHES 38.0

#define WORLD_SIZE (64.0 * 3.0)
#define MAX_HEIGHT (64.0 * 3.0)

void main()
{
	vec4 vecIn = vec4(vs_position_in, 1.0);
	worldPos = m * vecIn;
	
	// > Vagues
	// worldPos.z += sin(elapsed * 5 + worldPos.x * 0.125) * 1;

	// > Inception
	/*
	float normalizedX = (worldPos.x) / (WORLD_SIZE);
	float normalizedZ = (worldPos.z) / (MAX_HEIGHT);
	worldPos = vec4(
	 	MAX_HEIGHT * (1.0 - normalizedZ) * cos(normalizedX * 2.0 * 3.14159265359 + 3.14159265359 * 0.5 + elapsed * 0.25), 
	 	worldPos.y, 
	 	MAX_HEIGHT * (1.0 - normalizedZ) * sin(normalizedX * 2.0 * 3.14159265359 + 3.14159265359 * 0.5 + elapsed * 0.25), 
	 	worldPos.w
	);
	*/

	// > Effet planète
	/*
	vec3 relativePos = worldPos.xyz - camera_pos.xyz;
	float normalizedX = (relativePos.x) / (WORLD_SIZE);
	float normalizedY = (relativePos.y) / (WORLD_SIZE);
	float normalizedDist = length(vec2(normalizedX, normalizedY));
	float normalizedZ = (worldPos.z) / (MAX_HEIGHT);
	worldPos = vec4(
	 	worldPos.x,
	 	worldPos.y, 
		worldPos.z - (normalizedDist * normalizedDist) * 500,
		worldPos.w
	);
	*/
	
	gl_Position = (p * v) * worldPos;
	
	normal = (nmat * vec4(vs_normal_in, 1.0)).xyz; 

	uv = vs_uv_in;
	ao = vs_ao_in;

	//Couleur par défaut violet
	color = vec4(1.0,0.0,1.0,1.0);
	
	//Couleur fonction du type
	if(vs_type_in == CUBE_HERBE)
		color = vec4(0.2, 0.55, 0.2, 1);
	if(vs_type_in == CUBE_TERRE)
		color = vec4(0.4, 0.32, 0.21, 1);
	if(vs_type_in == CUBE_PIERRE)
		color = vec4(0.4, 0.4, 0.4, 1);
	if(vs_type_in == CUBE_EAU)
		color = vec4(0.0, 0.5, 1.0, 0.7);
	if(vs_type_in == CUBE_SABLE_01)
		color = vec4(0.75, 0.75, 0.65, 1.0);
	if(vs_type_in == CUBE_BRANCHES)
		color = vec4(0.075, 0.25, 0.1, 1);
	
	type = vs_type_in;
}
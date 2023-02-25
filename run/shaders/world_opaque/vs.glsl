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

#define CUBE_HERBE 0.0
#define CUBE_TERRE 1.0
#define CUBE_PIERRE 3.0
#define CUBE_SABLE_01 17.0
#define CUBE_LAINE_01 19.0

#define WORLD_SIZE (64.0 * 4.0)
#define MAX_HEIGHT (64.0 * 2.0 + 32.0)

void main()
{
	vec4 vecIn = vec4(vs_position_in, 1.0);
	worldPos = m * vecIn;
	
	// > Vagues
	// vecIn.z += sin(elapsed * 5 + worldPos.x * 0.125) * 1;
	// gl_Position = mvp * vecIn;

	// > Inception
	// float normalizedX = (worldPos.x) / (WORLD_SIZE);
	// float normalizedZ = (worldPos.z) / (MAX_HEIGHT);
	// worldPos = vec4(
	// 	MAX_HEIGHT * (1.0 - normalizedZ) * cos(normalizedX * 2.0 * 3.14159265359 + 3.14159265359 * 0.5), 
	// 	worldPos.y, 
	// 	MAX_HEIGHT * (1.0 - normalizedZ) * sin(normalizedX * 2.0 * 3.14159265359 + 3.14159265359 * 0.5), 
	// 	worldPos.w
	// );

	// > Effet planète
	// vec3 relativePos = worldPos.xyz - camera_pos.xyz;
	// float normalizedX = (relativePos.x) / (WORLD_SIZE);
	// float normalizedY = (relativePos.y) / (WORLD_SIZE);
	// float normalizedDist = length(vec2(normalizedX, normalizedY));
	// float normalizedZ = (worldPos.z) / (MAX_HEIGHT);
	// worldPos = vec4(
	// 	worldPos.x,
	// 	worldPos.y, 
	// 	worldPos.z - (normalizedDist * normalizedDist) * 1000,
	// 	worldPos.w
	// );
	
	gl_Position = (p * v) * worldPos;
	actualClipZ = gl_Position.z;
	
	normal = (nmat * vec4(vs_normal_in, 1.0)).xyz; 

	uv = vs_uv_in;
	ao = vs_ao_in;

	//Couleur par défaut violet
	color = vec4(1.0,0.0,1.0,1.0);
	
	//Couleur fonction du type
	if(vs_type_in == CUBE_HERBE)
		color = vec4(0.2, 0.55, 0.2, 1.0);
	if(vs_type_in == CUBE_TERRE)
		color = vec4(0.4, 0.32, 0.21, 1.0);
	if(vs_type_in == CUBE_PIERRE)
		color = vec4(0.4, 0.4, 0.4, 1.0);
	if(vs_type_in == CUBE_SABLE_01)
		color = vec4(0.75, 0.75, 0.65, 1.0);
	if(vs_type_in == CUBE_LAINE_01)
		color = vec4(1.0, 1.0, 1.0, 1.0);

	normalizedDistToCamera = clamp(length(worldPos.xyz - camera_pos) / (WORLD_SIZE * 0.5), 0.0, 1.0);
	normalizedDistToCamera = normalizedDistToCamera * normalizedDistToCamera;
}
#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec4 color;
in vec2 uv;
in float ao;
in float normalizedDistToCamera;
in float actualClipZ;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

uniform float elapsed;
uniform vec3 camera_pos;

uniform vec3 sun_color;
uniform vec3 sun_light_color;
uniform vec3 sun_direction;

uniform vec3 ambient_color;
uniform vec3 fog_color;

#define SHADOW_CASCADE_COUNT 4
uniform sampler2DShadow shadow_map[SHADOW_CASCADE_COUNT];
uniform float shadow_cascade_far[SHADOW_CASCADE_COUNT];
uniform float shadow_cascade_far_clip_z[SHADOW_CASCADE_COUNT];
uniform mat4 shadow_vp[SHADOW_CASCADE_COUNT];
uniform float inv_shadowmap_size;

layout(location=1) out vec4 normal_out;
layout(location=2) out vec4 color_out;

float getNoise(vec2 pos)
{
	float time = elapsed;
	vec2 source1 = vec2(-127.511, 153.421);
	vec2 source2 = vec2(-90.456, -30.7411);
	vec2 source3 = vec2(325.723, 80.342);
	vec2 source4 = vec2(20.3477, -60.4215);
	float noiseValue = 0;
	noiseValue = sin(length(pos - source1) * 1.63 - 1.25 * time) * 0.4;
	noiseValue += sin(length(pos - source2) * 1.19 - 1.02 * time) * 0.4;
	noiseValue += sin(length(pos - source3) * 3.51 - 2.14 * time) * 0.2;
	noiseValue += sin(length(pos - source4) * 2.71 - 3.57 * time) * 0.2;
	noiseValue = noiseValue * 0.02 - 0.125;

	return noiseValue;
}

vec3 getNoiseNormal(vec2 pos)
{
	float delta = 0.001;
	float corner1Noise = getNoise(pos + vec2(-delta, -delta));
	float corner2Noise = getNoise(pos + vec2(delta, -delta));
	float corner3Noise = getNoise(pos + vec2(-delta, delta));

	vec3 refVec1 = vec3(2*delta, corner2Noise - corner1Noise, 0); 
	vec3 refVec2 = vec3(0, corner3Noise - corner1Noise, 2*delta);
	
	return normalize(cross(refVec2, refVec1)).xzy;
}

float sampleShadow(vec2 uv, float refDepth, int cascade)
{
	return texture(shadow_map[cascade], vec3(uv, refDepth));
}

float getShadowValue()
{
	const float shadowBiases[SHADOW_CASCADE_COUNT] = float[SHADOW_CASCADE_COUNT]( 
		0.000125, 0.0004, 0.001, 0.00125
	);

	int cascadeIndex = SHADOW_CASCADE_COUNT - 1;
	for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
	{
		if (actualClipZ <= shadow_cascade_far_clip_z[i])
		{
			cascadeIndex = i;
			break;
		}
	}

	vec4 shadowClipPos = shadow_vp[cascadeIndex] * worldPos;
	vec2 shadowUV = (shadowClipPos.xy / shadowClipPos.w) * 0.5 + 0.5;
	float actualShadowDepth = (shadowClipPos.z / shadowClipPos.w) * 0.5 + 0.5;
	float biasedShadowDepth = actualShadowDepth - shadowBiases[cascadeIndex];

	float shadowValue = sampleShadow(shadowUV, biasedShadowDepth, cascadeIndex);
	return shadowValue;
}

void main()
{
	vec3 noiseNormal = getNoiseNormal(worldPos.xy);

	float shadowValue = getShadowValue();
	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	// fragAo = 1.0;

	vec3 viewVec = normalize(camera_pos - worldPos.xyz);
	vec3 sunDir = normalize(sun_direction);
	vec3 sunHalfVec = normalize(viewVec + sunDir);

	float sunLightDiffuse = max(0, dot(sunDir, noiseNormal));

	float sunLightSpecular;
	if (sunLightDiffuse > 0)
		sunLightSpecular = max(0, dot(sunHalfVec, noiseNormal));
	else
		sunLightSpecular = 0;
	sunLightSpecular = pow(sunLightSpecular, 250);

	float ambientAmount = 1.0 - sunLightDiffuse * 0.625;
	ambientAmount *= 0.75;

	vec3 baseColor = color.xyz;
	vec3 diffuse = baseColor * ((sunLightDiffuse * shadowValue) * sun_light_color) * fragAo;
	vec3 ambient = baseColor * (ambientAmount * ambient_color) * fragAo;
	vec4 specular = vec4((shadowValue * sunLightSpecular) * sun_color, shadowValue * sunLightSpecular);

	vec4 waterColor = vec4(diffuse + ambient, color.a) + specular;
	color_out = waterColor.rgba;

	// color_out = vec4(mix(litColor, fog_color, normalizedDistToCamera), color.a);

	/*
	float noiseValue = getNoise(worldPos.xy);
	noiseValue = noiseValue * 0.5 + 0.5;
	color_out = vec4(noiseValue, noiseValue, noiseValue, 1.0);
	*/

	normal_out = vec4(noiseNormal, 1.0);
}
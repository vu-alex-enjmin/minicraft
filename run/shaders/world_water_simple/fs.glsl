#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec4 color;
in vec2 uv;
in float ao;
in float normalizedDistToCamera;
in float actualClipZ;
in float fogFactor;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

uniform float elapsed;
uniform vec3 camera_pos;
uniform vec3 sun_color;
uniform vec3 sun_light_color;
uniform vec3 ambient_color;
uniform vec3 fog_color;
uniform vec3 sun_direction;
uniform vec2 near_far;
uniform float inv_shadowmap_size;

#define SHADOW_CASCADE_COUNT 4
uniform sampler2DShadow shadow_map[SHADOW_CASCADE_COUNT];
uniform float shadow_cascade_far[SHADOW_CASCADE_COUNT];
uniform float shadow_cascade_far_clip_z[SHADOW_CASCADE_COUNT];
uniform mat4 shadow_vp[SHADOW_CASCADE_COUNT];

out vec4 color_out;

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

vec3 getNoiseNormal(vec2 pos)
{
	float delta = 0.025;
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
	const vec3 worldUp = vec3(0, 0, 1);
	vec3 noiseNormal = getNoiseNormal(worldPos.xy);
	vec3 baseNormal = normalize(normal);
	vec3 actualNormal = mix(
		baseNormal, 
		noiseNormal, 
		max(0, dot(worldUp, baseNormal))
	);

	float shadowValue = getShadowValue();
	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	// fragAo = 1.0;

	vec3 viewVec = normalize(camera_pos - worldPos.xyz);
	vec3 sunDir = normalize(sun_direction);
	vec3 sunHalfVec = normalize(viewVec + sunDir); 

	float sunLightDiffuse = max(0, dot(sunDir, actualNormal));

	float sunLightSpecular;
	if (sunLightDiffuse > 0)
		sunLightSpecular = max(0, dot(sunHalfVec, actualNormal));
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
	color_out = waterColor;
	color_out = vec4(mix(color_out.rgb, fog_color, fogFactor), color_out.a);

	/*
	float noiseValue = getNoise(worldPos.xy);
	noiseValue = noiseValue * 0.5 + 0.5;
	color_out = vec4(vec3(noiseValue), 1.0);
	*/
}
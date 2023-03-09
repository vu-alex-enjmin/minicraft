#version 400

//Variables en entree
in vec4 worldPos;
in vec3 normal;
in vec2 uv;
in float ao;
in float normalizedDistToCamera;
in float actualClipZ;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

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

uniform sampler2D tex_atlas;

out vec4 color_out;

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
	vec4 texColor = textureLod(tex_atlas, uv, 0);
	if (texColor.a <= 1e-6)
	{
		discard;
	}

	float shadowValue = getShadowValue();

	vec3 viewVec = normalize(camera_pos - worldPos.xyz);
	vec3 sunDir = normalize(sun_direction);
	vec3 sunHalfVec = normalize(viewVec + sunDir); 

	float sunLightDiffuse = max(0, dot(sunDir, normal));
	sunLightDiffuse = min(sunLightDiffuse, shadowValue);

	float sunLightSpecular;
	if (sunLightDiffuse > 0)
		sunLightSpecular = max(0, dot(sunHalfVec, normal));
	else
		sunLightSpecular = 0;
	sunLightSpecular = pow(sunLightSpecular, 3) * 0.075;

	float ambientAmount = 1.0 - sunLightDiffuse * 0.625;
	ambientAmount *= 0.75;

	vec3 baseColor = texColor.rgb;
	vec3 diffuse = baseColor * ((sunLightDiffuse) * sun_light_color);
	vec3 ambient = baseColor * (ambientAmount * ambient_color);
	vec3 specular = (shadowValue * sunLightSpecular) * sun_color;

	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	// fragAo = 1.0;
	vec3 litColor = (ambient + diffuse + specular) * fragAo;
	// litColor = fragAo * (baseColor * 0.25 + baseColor * ambient_color * 0.75);
	color_out = vec4(litColor, 1.0);
	
	// color_out = vec4(baseColor.rgb, 1.0);
	/*
	color_out = vec4(fragAo, fragAo, fragAo, 1.0);
	if (actualClipZ <= shadow_cascade_far_clip_z[0])
		color_out *= vec4(1, 0.8, 0.8, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[1])
		color_out *= vec4(0.8, 1, 0.8, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[2])
		color_out *= vec4(0.8, 0.8, 1, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[3])
		color_out *= vec4(1, 1, 0.8, 1);
	color_out.rgb *= shadowValue;
	*/

	// color_out = vec4(mix(litColor, fog_color, normalizedDistToCamera), color.a);
	// color_out = vec4(ambient_color, 1);

	// color_out = vec4(uv, 0, 1);
	// color_out = vec4(sun_color, 1);
}
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

uniform vec3 camera_pos;
uniform vec3 sun_color;
uniform vec3 ambient_color;
uniform vec3 fog_color;
uniform vec3 sun_direction;
uniform vec2 near_far;

#define SHADOW_CASCADE_COUNT 4
#define SHADOWMAP_SIZE 1024
uniform sampler2D sun_shadow_map;
uniform float shadow_cascade_far[SHADOW_CASCADE_COUNT];
uniform float shadow_cascade_far_clip_z[SHADOW_CASCADE_COUNT];
uniform mat4 shadow_vp[SHADOW_CASCADE_COUNT];

out vec4 color_out;

vec2 adaptUVToCascade(vec2 baseUV, int cascade)
{
	vec2 adaptedUV = baseUV * 0.5;
	adaptedUV.x += 0.5 * (cascade % 2);
	adaptedUV.y += 0.5 * (cascade / 2);
	return adaptedUV;
}

float sampleShadow(vec2 uv, float refDepth)
{
	return (refDepth > texture2D(sun_shadow_map, uv).r) ? 0.0 : 1.0;
}

float getShadowValue()
{
	const float shadowBiases[SHADOW_CASCADE_COUNT] = float[SHADOW_CASCADE_COUNT]( 0.00005, 0.00015, 0.000425, 0.0015 );

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
	float actualShadowDepth = (shadowClipPos.z / shadowClipPos.w) * 0.5 + 0.5;
	actualShadowDepth -= shadowBiases[cascadeIndex];
	vec2 myUV = (shadowClipPos.xy / shadowClipPos.w) * 0.5 + 0.5;
	myUV = adaptUVToCascade(myUV, cascadeIndex);

	// vec2 baseShadowMapOffset = vec2(1.0, 1.0) / SHADOWMAP_SIZE;
	
	return sampleShadow(myUV, actualShadowDepth);
}

void main()
{
	float shadowValue = getShadowValue();

	vec3 viewVec = normalize(camera_pos - worldPos.xyz);
	vec3 sunDir = normalize(sun_direction);
	vec3 sunHalfVec = normalize(viewVec + sunDir); 

	float sunLightDiffuse = max(0, dot(sunDir, normal));

	float sunLightSpecular;
	if (sunLightDiffuse > 0)
		sunLightSpecular = max(0, dot(sunHalfVec, normal));
	else
		sunLightSpecular = 0;
	sunLightSpecular = pow(sunLightSpecular, 2) * 0.33;

	float ambientAmount = max(0.8, dot(normalize(vec3(-0.25, -0.5, 1.0)), normal)) * 0.5;

	sunLightDiffuse = 1.0f;
	vec3 baseColor = color.xyz;
	vec3 diffuse = baseColor * ((sunLightDiffuse * shadowValue) * sun_color);
	vec3 ambient = baseColor * (ambientAmount * ambient_color);
	vec3 specular = (shadowValue * sunLightSpecular) * sun_color;

	float fragAo = 1 - (1 - ao) * (1 - ao) * 0.75;
	// fragAo = 1.0;
	vec3 litColor = (ambient + diffuse + specular) * fragAo;
	// litColor = fragAo * (baseColor * 0.25 + baseColor * ambient_color * 0.75);
	color_out = vec4(litColor, color.a);
	

	// color_out = vec4(fragAo, fragAo, fragAo, color.a);
	// color_out = vec4(texture2D(sun_shadow_map, uv).r, 0, 0, 1);
	/*
	if (actualClipZ <= shadow_cascade_far_clip_z[0])
		color_out *= vec4(1, 0.8, 0.8, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[1])
		color_out *= vec4(0.8, 1, 0.8, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[2])
		color_out *= vec4(0.8, 0.8, 1, 1);
	else if (actualClipZ <= shadow_cascade_far_clip_z[3])
		color_out *= vec4(1, 1, 0.8, 1);
	*/

	// color_out = vec4(mix(litColor, fog_color, normalizedDistToCamera), color.a);
}
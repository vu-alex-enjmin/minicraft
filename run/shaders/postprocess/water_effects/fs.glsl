#version 400

in vec2 uv;

uniform sampler2D TexColor;
uniform sampler2D TexNormal;
uniform sampler2D TexWaterColor;
uniform sampler2D TexWaterAlpha;
uniform sampler2D TexDepth;

uniform vec2 near_far;

uniform mat4 v;
uniform mat4 p;
uniform mat4 inv_p;

uniform float screen_width;
uniform float screen_height;

uniform float refraction_intensity;
uniform float reflection_intensity;

out vec4 color_out;

const float thickness = 1.0;
const float marchStepLength = 0.125;
const float minRayStep = 0.125;
const int maxSteps = 75;
const int binarySearchSteps = 15;

float linearizeDepth(float z)
{
	float n = near_far.x; // camera z near
  	float f = near_far.y; // camera z far
  	return (2.0 * n) / (f + n - z * (f - n));
}

vec3 sampleViewPos(vec2 sampleUv)
{
	float z = texture(TexDepth, sampleUv).r * 2.0 - 1.0;
	vec4 clipPos = vec4(sampleUv * 2.0 - 1.0, z, 1.0);
	vec4 viewPos = inv_p * clipPos;
	
	return viewPos.xyz / viewPos.w;
}

vec3 binarySearch(vec3 marchStep, inout vec3 hitPos, inout float zDelta)
{
    float z;
    vec4 hitClipPos;
 
    for (int i = 0; i < binarySearchSteps; i++)
    {
        hitClipPos = p * vec4(hitPos, 1.0);
        hitClipPos.xy /= hitClipPos.w;
        hitClipPos.xy = hitClipPos.xy * 0.5 + 0.5;
 
        z = sampleViewPos(hitClipPos.xy).z;
		zDelta = hitPos.z - z;

        marchStep *= 0.5;
        if(zDelta > 0.0)
            hitPos += marchStep;
        else
            hitPos -= marchStep;
    }

    hitClipPos = p * vec4(hitPos, 0.75);
    hitClipPos.xy /= hitClipPos.w;
    hitClipPos.xy = hitClipPos.xy * 0.5 + 0.5;
 
    return vec3(hitClipPos.xy, z);
}

vec4 rayMarch(vec3 dir, inout vec3 hitPos, out float zDelta)
{
	vec3 marchStep = dir * marchStepLength;

	float z;
	vec4 hitClipPos;

	for (int i = 0; i < maxSteps; i++)
	{
		hitPos += marchStep;

		hitClipPos = p * vec4(hitPos, 1.0);
		hitClipPos.xy /= hitClipPos.w;
		hitClipPos.xy = hitClipPos.xy * 0.5 + 0.5;

		z = sampleViewPos(hitClipPos.xy).z;

		zDelta = hitPos.z - z;

		if ((marchStep.z - zDelta) < thickness && zDelta <= 0.0)
		{
			vec4 res = vec4(binarySearch(marchStep, hitPos, zDelta), 1.0);
			return res;
		}
	}

	return vec4(hitClipPos.xy, z, 0.0);
}

void main (void)
{
	vec4 normal = texture(TexNormal, uv);
	vec4 color;

	// Refraction
	vec2 texelOffset = vec2(1, 1) / vec2(screen_width, screen_height);
	float offsetAmount = 600.0 * refraction_intensity;
	vec2 offsetUV = uv + normal.xy * texelOffset * offsetAmount;
	if (length(texture(TexNormal, offsetUV).xyz) > 0)
	{
		color = texture(TexColor , offsetUV);
	}
	else
	{
		color = texture(TexColor , uv);
	}
	
	// Water color
	vec4 waterColor = texture(TexWaterColor, uv);
	float waterAlpha = waterColor.a;
	color_out = vec4(color.rgb * (1-waterAlpha) + waterColor.rgb * waterAlpha, 1.0);

	// Reflection
	// Mostly adapted from https://imanolfotia.com/blog/1
	if (length(normal.xyz) > 0)
	{
		vec3 viewNormal = (v * vec4(normal.xyz, 0.0)).xyz;
		vec3 viewPos = sampleViewPos(uv);

		vec3 normalizedViewPos = normalize(viewPos);
		vec3 reflected = normalize(reflect(normalizedViewPos, normalize(viewNormal)));
		vec3 hitPos = viewPos;
		float zDelta;

		vec4 reflectionPos = rayMarch(reflected * max(minRayStep, -viewPos.z), hitPos, zDelta);

		if (reflectionPos.z <= viewPos.z)
		{
			vec2 ssrUv = reflectionPos.xy;
		
			vec2 uvFadeout = 1.0 - (abs(2 * ssrUv - 1.0));
			uvFadeout = clamp(uvFadeout, 0, 1);
			uvFadeout = 1.0 - (1.0 - uvFadeout) * (1.0 - uvFadeout) * (1.0 - uvFadeout) * (1.0 - uvFadeout);

			float linearDepth = linearizeDepth(texture(TexDepth, ssrUv).r);
			if (linearDepth < 1.0 - 1e-4)
			{
				vec3 ssrColor = texture(TexColor, ssrUv).rgb;

				float depthFade = (1.0 - linearDepth);
				depthFade = depthFade;

				float opacity = 
					reflection_intensity *
					depthFade * 
					clamp(dot(reflected, normalizedViewPos), 0, 1) * 
					min(uvFadeout.x, uvFadeout.y);

				color_out.xyz = mix(color_out.xyz, ssrColor, opacity);
			}
		}
	}
}
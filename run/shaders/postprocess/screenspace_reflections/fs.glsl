#version 400

in vec2 uv;

uniform sampler2D TexColor;
uniform sampler2D TexNormal;
uniform sampler2D TexWaterColor;
uniform sampler2D TexWaterAlpha;
uniform sampler2D TexDepth;

uniform vec2 near_far;

uniform mat4 v;
uniform mat4 inv_v;
uniform mat4 p;
uniform mat4 inv_p;

uniform float screen_width;
uniform float screen_height;

uniform float intensity;

out vec4 color_out;

void main (void)
{
	vec4 color;
	vec4 normal = texture(TexNormal, uv);
	
	vec2 texelOffset = vec2(1, 1) / vec2(screen_width, screen_height);
	float offsetAmount = 600.0 * intensity;
	vec2 offsetUV = uv + normal.xy * texelOffset * offsetAmount;
	if (length(texture(TexNormal, offsetUV).xyz) > 0)
	{
		color = texture(TexColor , offsetUV);
	}
	else
	{
		color = texture(TexColor , uv);
	}

	color_out = color;
	
	vec4 waterColor = texture(TexWaterColor, uv);
	float waterAlpha = texture(TexWaterAlpha, uv).r;
	color_out = color * (1-waterAlpha) + waterColor * waterAlpha;
}
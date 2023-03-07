#version 400

in vec2 uv;

uniform sampler2D TexColor;
uniform sampler2D TexNormal;

uniform float intensity;
uniform float radius;

out vec4 color_out;

void main (void)
{
	vec4 color = texture( TexColor , uv );

	float vignette = length((uv - 0.5) * 2.0) / radius;
	vignette = vignette * vignette;
	
	color.rgb = mix(color.rgb, vec3(0,0,0), vignette * intensity);
	color_out = vec4(color.rgb, 1.0);
}
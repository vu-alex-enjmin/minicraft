#version 400

in vec2 uv;

uniform sampler2D TexColor;

out vec4 color_out;

// Params :
// Radius, Intensity

void main (void)
{
	float radius = 1.75;
	float intensity = 1.0;

	vec4 color = texture2D( TexColor , uv );

	float vignette = length((uv - 0.5) * 2.0) / radius;
	vignette = vignette * vignette;
	
	color.rgb = mix(color.rgb, vec3(0,0,0), vignette * intensity);
	color_out = vec4(color.rgb, 1.0);
}
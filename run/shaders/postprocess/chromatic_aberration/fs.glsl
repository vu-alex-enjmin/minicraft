#version 400

in vec2 uv;

uniform sampler2D TexColor;

uniform float horizontal_intensity;
uniform float vertical_intensity;

// Params :
// Horizontal Intensity
// Vertical Intensity

vec2 getDistortedUv(float horizontalIntensity, float verticalIntensity, float multiplier)
{
	vec2 intensities = 2 * (uv - 0.5);
	intensities *= intensities;
	intensities *= vec2(horizontalIntensity, verticalIntensity);

	vec2 distortedUv;
	distortedUv.x = (uv.x - 0.5) / mix(1.0, multiplier, intensities.x) + 0.5;
	distortedUv.y = (uv.y - 0.5) / mix(1.0, multiplier, intensities.y) + 0.5;

	return distortedUv;
}

void main (void)
{
	float red = texture(TexColor, getDistortedUv(horizontal_intensity, vertical_intensity, 1.0)).r;
	float green = texture(TexColor, getDistortedUv(horizontal_intensity, vertical_intensity, 1.025)).g;
	float blue = texture(TexColor, getDistortedUv(horizontal_intensity, vertical_intensity, 1.05)).b;

	color_out = vec4(red, green, blue, 1.0);
}
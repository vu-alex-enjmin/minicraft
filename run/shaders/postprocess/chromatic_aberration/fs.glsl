#version 400

in vec2 uv;

uniform sampler2D TexColor;

out vec4 color_out;

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
	float horizontalIntensity = 1.0;
	float verticalIntensity = 0.75;

	float red = texture2D(TexColor, getDistortedUv(horizontalIntensity, verticalIntensity, 1.0)).r;
	float green = texture2D(TexColor, getDistortedUv(horizontalIntensity, verticalIntensity, 1.025)).g;
	float blue = texture2D(TexColor, getDistortedUv(horizontalIntensity, verticalIntensity, 1.05)).b;

	color_out = vec4(red, green, blue, 1.0);
}
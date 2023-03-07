#version 400

in vec2 uv;

uniform sampler2D TexColor;
uniform sampler2D TexDepth;

uniform vec2 near_far;

uniform mat4 v;
uniform mat4 inv_v;
uniform mat4 p;
uniform mat4 inv_p;

uniform float screen_width;
uniform float screen_height;

out vec4 color_out;

// Params :
// ???

void main (void)
{
	vec4 color = texture(TexColor , uv);
	color_out = color;
}
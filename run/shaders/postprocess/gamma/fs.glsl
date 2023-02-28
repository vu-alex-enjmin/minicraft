#version 400

in vec2 uv;

uniform sampler2D TexColor;

out vec4 color_out;

void main (void)
{
	vec4 color = texture( TexColor , uv );

    //Gamma correction
    color.r = pow(color.r,1.0/2.2);
    color.g = pow(color.g,1.0/2.2);
    color.b = pow(color.b,1.0/2.2);

	color_out = vec4(color.rgb, 1.0);
}
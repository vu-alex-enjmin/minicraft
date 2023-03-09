#version 400

uniform sampler2D tex_atlas;

in vec2 uv;

void main()
{
	vec4 texColor = textureLod(tex_atlas, uv, 0);
	if (texColor.a <= 1e-6)
	{
		discard;
	}
}

#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) out vec4 outColour;
layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTexCord;
void main()
{
	//outColour = vec4(texture(textureSampler, fragTexCord).rgb / fragColour, 1.0);
	outColour = vec4(1.0, 1.0, 1.0, 1.0);
}
// Copyright (c) Giovanni Giacomo. All Rights Reserved.
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec3 eyeDirection;

layout(set = 0, binding = 0) uniform VertexUniform
{
	mat4 projection;
} vu;

layout(push_constant) uniform VertexPush
{
	mat4 view;
	mat4 model;
} vp;

void main()
{
	gl_Position = vu.projection * vp.view * vp.model * vec4(position, 1.0);

	// Perform calculations and prepare data for the fragment shader.
	fragColor = color;
	fragNormal = mat3(transpose(inverse(vp.model))) * normal;
	fragPosition = (vp.model * vec4(position, 1.0)).xyz;
	eyeDirection = -transpose(vp.view)[2].xyz;
}

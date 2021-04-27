// Copyright (c) Giovanni Giacomo. All Rights Reserved.
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinate;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTextureCoordinate;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 projection;
} cu;

layout(push_constant) uniform GBufferPush
{
	mat4 view;
	mat4 model;
} gbp;

void main()
{
	gl_Position = cu.projection * gbp.view * gbp.model * vec4(position, 1.0);

	// Perform calculations and prepare data for the fragment shader.
	fragPosition = (gbp.model * vec4(position, 1.0)).xyz;
	fragNormal = mat3(transpose(inverse(gbp.model))) * normal;
	fragTextureCoordinate = textureCoordinate;
}

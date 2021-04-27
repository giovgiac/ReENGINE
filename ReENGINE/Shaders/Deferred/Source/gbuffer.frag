// Copyright (c) Giovanni Giacomo. All Rights Reserved.
#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTextureCoordinate;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

// Bindings belonging to the Uniform Buffer Descriptor Set.
layout(set = 0, binding = 1) uniform MaterialUniform
{
	float specularPower;
	float specularStrength;
} mu;

// Bindings belonging to the Texture Sampler Descriptor Set.

layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;

void main()
{
	// Calculate the final color of the fragment.
	outColor.rgb = texture(diffuseSampler, fragTextureCoordinate).rgb;
	outColor.a = mu.specularStrength;
	outNormal = vec4(fragNormal, 1.0);
	outPosition = vec4(fragPosition, 1.0);
}

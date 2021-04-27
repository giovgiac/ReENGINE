// Copyright (c) Giovanni Giacomo. All Rights Reserved.
#version 450

// G-buffer input attachments.
layout(set = 0, input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout(set = 0, input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;
layout(set = 0, input_attachment_index = 2, binding = 2) uniform subpassInput inputNormal;
layout(set = 0, input_attachment_index = 3, binding = 3) uniform subpassInput inputPosition;

layout (location = 0) out vec4 outColor;

// Constant configuration values.
const uint MAX_POINT_LIGHTS = 32;
const uint MAX_SPOT_LIGHTS = 32;
const float SPECULAR_POWER = 16.0f;

// Global variables loaded from G-buffer attachments.
vec3 fragColor;
vec3 fragNormal;
vec3 fragPosition;
float specularStrength;

// Structure definitions for lighting.
struct Light
{
	vec3 color;
	float ambientStrength;
	float diffuseStrength;
};

struct DirectionalLight
{
	Light base;
	vec3 direction;
};

struct PointLight
{
	Light base;
	vec3 position;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

struct SpotLight
{
	PointLight base;
	vec3 direction;
	float cutoffAngle;
};

// Bindings belonging to the Lighting Descriptor Set.
layout(set = 1, binding = 0) uniform LightingUniform
{
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];
	uint pointLightCount;
	uint spotLightCount;
} lu;

layout(push_constant) uniform LightingPush
{
	vec3 eyePosition;
} lp;

// Helper functions for calculating the Phong Lighting Model.

vec3 CalculateLightByDirection(Light base, vec3 direction)
{
	// Calculate ambient light parameters.
	vec3 ambientColor = base.color * base.ambientStrength;
	
	// Calculate diffuse light parameters.
	float diffuseFactor = max(dot(normalize(fragNormal), normalize(direction)), 0.0);
	vec3 diffuseColor = diffuseFactor * base.diffuseStrength * base.color;

	// Calculate specular light parameters.
	vec3 specularColor = vec3(0, 0, 0);
	if (diffuseFactor > 0.0)
	{
		vec3 fragToEye = normalize(fragPosition - lp.eyePosition);
		vec3 reflectDirection = normalize(-reflect(normalize(direction), normalize(fragNormal)));
		float specularFactor = dot(fragToEye, reflectDirection);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, SPECULAR_POWER);
			specularColor = specularFactor * specularStrength * base.color;
		}
	}

	return (ambientColor + diffuseColor + specularColor);
}

vec3 CalculateDirectionalLight()
{
	return CalculateLightByDirection(lu.directionalLight.base, lu.directionalLight.direction);
}

vec3 CalculatePointLight(PointLight pointLight)
{
	// Calculate the direction and distance between the fragment and light source.
	vec3 direction = pointLight.position - fragPosition;
	float distance = length(direction);

	// Calculate the light and attenuation.
	vec3 lightColor = CalculateLightByDirection(pointLight.base, normalize(direction));
	float attenuation = pointLight.quadraticAttenuation * distance * distance + 
						pointLight.linearAttenuation * distance + 
						pointLight.constantAttenuation;

	return (lightColor / attenuation);
}

vec3 CalculateSpotLight(SpotLight spotLight)
{
	// Calculate the direction between the fragment and light source, and calculate the factor.
	vec3 rayDirection = normalize(fragPosition - spotLight.base.position);
	float spotFactor = dot(rayDirection, normalize(spotLight.direction));

	if (spotFactor > spotLight.cutoffAngle)
	{
		// Calculate the color and the fade of the spot light.
		vec3 lightColor = CalculatePointLight(spotLight.base);
		float spotFade = 1.0f - (1.0f - spotFactor) * (1.0f / (1.0f - spotLight.cutoffAngle));

		return spotFade * lightColor;
	}
	else
	{
		return vec3(0.0, 0.0, 0.0);
	}
}

vec3 CalculatePointLights()
{
	vec3 totalColor = vec3(0.0, 0.0, 0.0);
	for (uint i = 0; i < lu.pointLightCount; i++)
	{
		totalColor += CalculatePointLight(lu.pointLights[i]);
	}

	return totalColor;
}

vec3 CalculateSpotLights()
{
	vec3 totalColor = vec3(0.0, 0.0, 0.0);
	for (uint i = 0; i < lu.spotLightCount; i++)
	{
		totalColor += CalculateSpotLight(lu.spotLights[i]);
	}

	return totalColor;
}

void main()
{
	// Load the fragment data from input attachments.
	fragColor = subpassLoad(inputColor).rgb;
	fragNormal = subpassLoad(inputNormal).rgb;
	fragPosition = subpassLoad(inputPosition).rgb;

	// Load the material data from input attachments.
	specularStrength = subpassLoad(inputColor).a;

	// Calculate the light color to apply to the fragment.
	vec3 lightColor = CalculateDirectionalLight() + CalculatePointLights() + CalculateSpotLights();

	// Calculate the final color of the fragment.
	outColor = vec4(fragColor * lightColor, 1.0);
}

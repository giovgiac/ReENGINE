// Copyright (c) Giovanni Giacomo. All Rights Reserved.
#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTextureCoordinate;
layout(location = 3) in vec3 eyeDirection;

layout(location = 0) out vec4 outColor;

// Constant configuration values.
const uint MAX_POINT_LIGHTS = 4;
const uint MAX_SPOT_LIGHTS = 4;

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

// Structure definitions for object's information.
struct Material
{
	float specularPower;
	float specularStrength;
};

// Bindings belonging to the Uniform Buffer Descriptor Set.
layout(set = 0, binding = 1) uniform FragmentUniform
{
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];
	uint pointLightCount;
	uint spotLightCount;
} fu;

layout(set = 0, binding = 2) uniform FragmentDynamicUniform
{
	Material material;
} fdu;

// Bindings belonging to the Texture Sampler Descriptor Set.

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

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
		vec3 fragToEye = normalize(fragNormal - eyeDirection);
		vec3 reflectDirection = normalize(-reflect(normalize(direction), normalize(fragNormal)));
		float specularFactor = dot(fragToEye, reflectDirection);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, fdu.material.specularPower);
			specularColor = specularFactor * fdu.material.specularStrength * base.color;
		}
	}

	return (ambientColor + diffuseColor + specularColor);
}

vec3 CalculateDirectionalLight()
{
	return CalculateLightByDirection(fu.directionalLight.base, fu.directionalLight.direction);
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
	for (uint i = 0; i < fu.pointLightCount; i++)
	{
		totalColor += CalculatePointLight(fu.pointLights[i]);
	}

	return totalColor;
}

vec3 CalculateSpotLights()
{
	vec3 totalColor = vec3(0.0, 0.0, 0.0);
	for (uint i = 0; i < fu.spotLightCount; i++)
	{
		totalColor += CalculateSpotLight(fu.spotLights[i]);
	}

	return totalColor;
}

void main()
{
	// Calculate the light color to apply to the fragment.
	vec3 lightColor = CalculateDirectionalLight() + CalculatePointLights() + CalculateSpotLights();

	// Calculate the final color of the fragment.
	outColor = texture(textureSampler, fragTextureCoordinate) * vec4(lightColor, 1.0);
}

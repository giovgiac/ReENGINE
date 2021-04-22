/**
 * Color.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

#ifdef PLATFORM_USE_VULKAN
// Forward declaration of Vulkan's clear color.
typedef union VkClearColorValue;
#endif // PLATFORM_USE_VULKAN

namespace Re 
{
	namespace Math
	{
		/*
		 * @brief This data type is intended for hold RGBA colors. It uses floating-point numbers to represent the channels.
		 *
		 */
		struct Color
		{
			f32 Red;
			f32 Green;
			f32 Blue;
			f32 Alpha;

			/*
			 * @brief This default constructor initializes the color to the white color.
			 *
			 */
			Color()
				: Red(1.0f), Green(1.0f), Blue(1.0f), Alpha(1.0f) {}

			/*
			 * @brief This constructor initializes a color given the individual values of the channels.
			 *
			 * @param InRed: the value of the red channel.
			 * @param InGreen: the value of the green channel.
			 * @param InBlue: the value of the blue channel.
			 * @param InAlpha: the value of the alpha channel.
			 *
			 */
			Color(f32 InRed, f32 InGreen, f32 InBlue, f32 InAlpha)
				: Red(InRed), Green(InGreen), Blue(InBlue), Alpha(InAlpha) {}

			#ifdef PLATFORM_USE_VULKAN
			operator VkClearColorValue() const;
			#endif // PLATFORM_USE_VULKAN
		};

		namespace Colors {
			static const Color AliceBlue = { 0.941176534f, 0.972549081f, 1.000000000f, 1.000000000f };
			static const Color AntiqueWhite = { 0.980392218f, 0.921568692f, 0.843137324f, 1.000000000f };
			static const Color Aqua = { 0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const Color Aquamarine = { 0.498039246f, 1.000000000f, 0.831372619f, 1.000000000f };
			static const Color Azure = { 0.941176534f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const Color Beige = { 0.960784376f, 0.960784376f, 0.862745166f, 1.000000000f };
			static const Color Bisque = { 1.000000000f, 0.894117713f, 0.768627524f, 1.000000000f };
			static const Color Black = { 0.000000000f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const Color BlanchedAlmond = { 1.000000000f, 0.921568692f, 0.803921640f, 1.000000000f };
			static const Color Blue = { 0.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const Color BlueViolet = { 0.541176498f, 0.168627456f, 0.886274576f, 1.000000000f };
			static const Color Brown = { 0.647058845f, 0.164705887f, 0.164705887f, 1.000000000f };
			static const Color BurlyWood = { 0.870588303f, 0.721568644f, 0.529411793f, 1.000000000f };
			static const Color CadetBlue = { 0.372549027f, 0.619607866f, 0.627451003f, 1.000000000f };
			static const Color Chartreuse = { 0.498039246f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const Color Chocolate = { 0.823529482f, 0.411764741f, 0.117647067f, 1.000000000f };
			static const Color Coral = { 1.000000000f, 0.498039246f, 0.313725501f, 1.000000000f };
			static const Color CornflowerBlue = { 0.392156899f, 0.584313750f, 0.929411829f, 1.000000000f };
			static const Color Cornsilk = { 1.000000000f, 0.972549081f, 0.862745166f, 1.000000000f };
			static const Color Crimson = { 0.862745166f, 0.078431375f, 0.235294133f, 1.000000000f };
			static const Color Cyan = { 0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const Color DarkBlue = { 0.000000000f, 0.000000000f, 0.545098066f, 1.000000000f };
			static const Color DarkCyan = { 0.000000000f, 0.545098066f, 0.545098066f, 1.000000000f };
			static const Color DarkGoldenrod = { 0.721568644f, 0.525490224f, 0.043137256f, 1.000000000f };
			static const Color DarkGray = { 0.662745118f, 0.662745118f, 0.662745118f, 1.000000000f };
			static const Color DarkGreen = { 0.000000000f, 0.392156899f, 0.000000000f, 1.000000000f };
			static const Color DarkKhaki = { 0.741176486f, 0.717647076f, 0.419607878f, 1.000000000f };
			static const Color DarkMagenta = { 0.545098066f, 0.000000000f, 0.545098066f, 1.000000000f };
			static const Color DarkOliveGreen = { 0.333333343f, 0.419607878f, 0.184313729f, 1.000000000f };
			static const Color DarkOrange = { 1.000000000f, 0.549019635f, 0.000000000f, 1.000000000f };
			static const Color DarkOrchid = { 0.600000024f, 0.196078449f, 0.800000072f, 1.000000000f };
			static const Color DarkRed = { 0.545098066f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const Color DarkSalmon = { 0.913725555f, 0.588235319f, 0.478431404f, 1.000000000f };
			static const Color DarkSeaGreen = { 0.560784340f, 0.737254918f, 0.545098066f, 1.000000000f };
			static const Color DarkSlateBlue = { 0.282352954f, 0.239215702f, 0.545098066f, 1.000000000f };
			static const Color DarkSlateGray = { 0.184313729f, 0.309803933f, 0.309803933f, 1.000000000f };
			static const Color DarkTurquoise = { 0.000000000f, 0.807843208f, 0.819607913f, 1.000000000f };
			static const Color DarkViolet = { 0.580392182f, 0.000000000f, 0.827451050f, 1.000000000f };
			static const Color DeepPink = { 1.000000000f, 0.078431375f, 0.576470613f, 1.000000000f };
			static const Color DeepSkyBlue = { 0.000000000f, 0.749019623f, 1.000000000f, 1.000000000f };
			static const Color DimGray = { 0.411764741f, 0.411764741f, 0.411764741f, 1.000000000f };
			static const Color DodgerBlue = { 0.117647067f, 0.564705908f, 1.000000000f, 1.000000000f };
			static const Color Firebrick = { 0.698039234f, 0.133333340f, 0.133333340f, 1.000000000f };
			static const Color FloralWhite = { 1.000000000f, 0.980392218f, 0.941176534f, 1.000000000f };
			static const Color ForestGreen = { 0.133333340f, 0.545098066f, 0.133333340f, 1.000000000f };
			static const Color Fuchsia = { 1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const Color Gainsboro = { 0.862745166f, 0.862745166f, 0.862745166f, 1.000000000f };
			static const Color GhostWhite = { 0.972549081f, 0.972549081f, 1.000000000f, 1.000000000f };
			static const Color Gold = { 1.000000000f, 0.843137324f, 0.000000000f, 1.000000000f };
			static const Color Goldenrod = { 0.854902029f, 0.647058845f, 0.125490203f, 1.000000000f };
			static const Color Gray = { 0.501960814f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const Color Green = { 0.000000000f, 0.501960814f, 0.000000000f, 1.000000000f };
			static const Color GreenYellow = { 0.678431392f, 1.000000000f, 0.184313729f, 1.000000000f };
			static const Color Honeydew = { 0.941176534f, 1.000000000f, 0.941176534f, 1.000000000f };
			static const Color HotPink = { 1.000000000f, 0.411764741f, 0.705882370f, 1.000000000f };
			static const Color IndianRed = { 0.803921640f, 0.360784322f, 0.360784322f, 1.000000000f };
			static const Color Indigo = { 0.294117659f, 0.000000000f, 0.509803951f, 1.000000000f };
			static const Color Ivory = { 1.000000000f, 1.000000000f, 0.941176534f, 1.000000000f };
			static const Color Khaki = { 0.941176534f, 0.901960850f, 0.549019635f, 1.000000000f };
			static const Color Lavender = { 0.901960850f, 0.901960850f, 0.980392218f, 1.000000000f };
			static const Color LavenderBlush = { 1.000000000f, 0.941176534f, 0.960784376f, 1.000000000f };
			static const Color LawnGreen = { 0.486274540f, 0.988235354f, 0.000000000f, 1.000000000f };
			static const Color LemonChiffon = { 1.000000000f, 0.980392218f, 0.803921640f, 1.000000000f };
			static const Color LightBlue = { 0.678431392f, 0.847058892f, 0.901960850f, 1.000000000f };
			static const Color LightCoral = { 0.941176534f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const Color LightCyan = { 0.878431439f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const Color LightGoldenrodYellow = { 0.980392218f, 0.980392218f, 0.823529482f, 1.000000000f };
			static const Color LightGreen = { 0.564705908f, 0.933333397f, 0.564705908f, 1.000000000f };
			static const Color LightGray = { 0.827451050f, 0.827451050f, 0.827451050f, 1.000000000f };
			static const Color LightPink = { 1.000000000f, 0.713725507f, 0.756862819f, 1.000000000f };
			static const Color LightSalmon = { 1.000000000f, 0.627451003f, 0.478431404f, 1.000000000f };
			static const Color LightSeaGreen = { 0.125490203f, 0.698039234f, 0.666666687f, 1.000000000f };
			static const Color LightSkyBlue = { 0.529411793f, 0.807843208f, 0.980392218f, 1.000000000f };
			static const Color LightSlateGray = { 0.466666698f, 0.533333361f, 0.600000024f, 1.000000000f };
			static const Color LightSteelBlue = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
			static const Color LightYellow = { 1.000000000f, 1.000000000f, 0.878431439f, 1.000000000f };
			static const Color Lime = { 0.000000000f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const Color LimeGreen = { 0.196078449f, 0.803921640f, 0.196078449f, 1.000000000f };
			static const Color Linen = { 0.980392218f, 0.941176534f, 0.901960850f, 1.000000000f };
			static const Color Magenta = { 1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const Color Maroon = { 0.501960814f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const Color MediumAquamarine = { 0.400000036f, 0.803921640f, 0.666666687f, 1.000000000f };
			static const Color MediumBlue = { 0.000000000f, 0.000000000f, 0.803921640f, 1.000000000f };
			static const Color MediumOrchid = { 0.729411781f, 0.333333343f, 0.827451050f, 1.000000000f };
			static const Color MediumPurple = { 0.576470613f, 0.439215720f, 0.858823597f, 1.000000000f };
			static const Color MediumSeaGreen = { 0.235294133f, 0.701960802f, 0.443137288f, 1.000000000f };
			static const Color MediumSlateBlue = { 0.482352972f, 0.407843173f, 0.933333397f, 1.000000000f };
			static const Color MediumSpringGreen = { 0.000000000f, 0.980392218f, 0.603921592f, 1.000000000f };
			static const Color MediumTurquoise = { 0.282352954f, 0.819607913f, 0.800000072f, 1.000000000f };
			static const Color MediumVioletRed = { 0.780392230f, 0.082352944f, 0.521568656f, 1.000000000f };
			static const Color MidnightBlue = { 0.098039225f, 0.098039225f, 0.439215720f, 1.000000000f };
			static const Color MintCream = { 0.960784376f, 1.000000000f, 0.980392218f, 1.000000000f };
			static const Color MistyRose = { 1.000000000f, 0.894117713f, 0.882353008f, 1.000000000f };
			static const Color Moccasin = { 1.000000000f, 0.894117713f, 0.709803939f, 1.000000000f };
			static const Color NavajoWhite = { 1.000000000f, 0.870588303f, 0.678431392f, 1.000000000f };
			static const Color Navy = { 0.000000000f, 0.000000000f, 0.501960814f, 1.000000000f };
			static const Color OldLace = { 0.992156923f, 0.960784376f, 0.901960850f, 1.000000000f };
			static const Color Olive = { 0.501960814f, 0.501960814f, 0.000000000f, 1.000000000f };
			static const Color OliveDrab = { 0.419607878f, 0.556862772f, 0.137254909f, 1.000000000f };
			static const Color Orange = { 1.000000000f, 0.647058845f, 0.000000000f, 1.000000000f };
			static const Color OrangeRed = { 1.000000000f, 0.270588249f, 0.000000000f, 1.000000000f };
			static const Color Orchid = { 0.854902029f, 0.439215720f, 0.839215755f, 1.000000000f };
			static const Color PaleGoldenrod = { 0.933333397f, 0.909803987f, 0.666666687f, 1.000000000f };
			static const Color PaleGreen = { 0.596078455f, 0.984313786f, 0.596078455f, 1.000000000f };
			static const Color PaleTurquoise = { 0.686274529f, 0.933333397f, 0.933333397f, 1.000000000f };
			static const Color PaleVioletRed = { 0.858823597f, 0.439215720f, 0.576470613f, 1.000000000f };
			static const Color PapayaWhip = { 1.000000000f, 0.937254965f, 0.835294187f, 1.000000000f };
			static const Color PeachPuff = { 1.000000000f, 0.854902029f, 0.725490212f, 1.000000000f };
			static const Color Peru = { 0.803921640f, 0.521568656f, 0.247058839f, 1.000000000f };
			static const Color Pink = { 1.000000000f, 0.752941251f, 0.796078503f, 1.000000000f };
			static const Color Plum = { 0.866666734f, 0.627451003f, 0.866666734f, 1.000000000f };
			static const Color PowderBlue = { 0.690196097f, 0.878431439f, 0.901960850f, 1.000000000f };
			static const Color Purple = { 0.501960814f, 0.000000000f, 0.501960814f, 1.000000000f };
			static const Color Red = { 1.000000000f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const Color RosyBrown = { 0.737254918f, 0.560784340f, 0.560784340f, 1.000000000f };
			static const Color RoyalBlue = { 0.254901975f, 0.411764741f, 0.882353008f, 1.000000000f };
			static const Color SaddleBrown = { 0.545098066f, 0.270588249f, 0.074509807f, 1.000000000f };
			static const Color Salmon = { 0.980392218f, 0.501960814f, 0.447058856f, 1.000000000f };
			static const Color SandyBrown = { 0.956862807f, 0.643137276f, 0.376470625f, 1.000000000f };
			static const Color SeaGreen = { 0.180392161f, 0.545098066f, 0.341176480f, 1.000000000f };
			static const Color SeaShell = { 1.000000000f, 0.960784376f, 0.933333397f, 1.000000000f };
			static const Color Sienna = { 0.627451003f, 0.321568638f, 0.176470593f, 1.000000000f };
			static const Color Silver = { 0.752941251f, 0.752941251f, 0.752941251f, 1.000000000f };
			static const Color SkyBlue = { 0.529411793f, 0.807843208f, 0.921568692f, 1.000000000f };
			static const Color SlateBlue = { 0.415686309f, 0.352941185f, 0.803921640f, 1.000000000f };
			static const Color SlateGray = { 0.439215720f, 0.501960814f, 0.564705908f, 1.000000000f };
			static const Color Snow = { 1.000000000f, 0.980392218f, 0.980392218f, 1.000000000f };
			static const Color SpringGreen = { 0.000000000f, 1.000000000f, 0.498039246f, 1.000000000f };
			static const Color SteelBlue = { 0.274509817f, 0.509803951f, 0.705882370f, 1.000000000f };
			static const Color Tan = { 0.823529482f, 0.705882370f, 0.549019635f, 1.000000000f };
			static const Color Teal = { 0.000000000f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const Color Thistle = { 0.847058892f, 0.749019623f, 0.847058892f, 1.000000000f };
			static const Color Tomato = { 1.000000000f, 0.388235331f, 0.278431386f, 1.000000000f };
			static const Color Transparent = { 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f };
			static const Color Turquoise = { 0.250980407f, 0.878431439f, 0.815686345f, 1.000000000f };
			static const Color Violet = { 0.933333397f, 0.509803951f, 0.933333397f, 1.000000000f };
			static const Color Wheat = { 0.960784376f, 0.870588303f, 0.701960802f, 1.000000000f };
			static const Color White = { 1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const Color WhiteSmoke = { 0.960784376f, 0.960784376f, 0.960784376f, 1.000000000f };
			static const Color Yellow = { 1.000000000f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const Color YellowGreen = { 0.603921592f, 0.803921640f, 0.196078449f, 1.000000000f };
		}
	}
}

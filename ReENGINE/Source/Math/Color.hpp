/**
 * Color.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re 
{
	namespace Math
	{
		/*
		 * @brief This data type is intended for hold RGBA colors. It uses floating-point numbers to represent the channels.
		 *
		 */
		struct NColor
		{
			f32 Red;
			f32 Green;
			f32 Blue;
			f32 Alpha;

			/*
			 * @brief This default constructor initializes the color to the white color.
			 *
			 */
			NColor()
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
			NColor(f32 InRed, f32 InGreen, f32 InBlue, f32 InAlpha)
				: Red(InRed), Green(InGreen), Blue(InBlue), Alpha(InAlpha) {}
		};

		namespace Colors {
			static const NColor AliceBlue = { 0.941176534f, 0.972549081f, 1.000000000f, 1.000000000f };
			static const NColor AntiqueWhite = { 0.980392218f, 0.921568692f, 0.843137324f, 1.000000000f };
			static const NColor Aqua = { 0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const NColor Aquamarine = { 0.498039246f, 1.000000000f, 0.831372619f, 1.000000000f };
			static const NColor Azure = { 0.941176534f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const NColor Beige = { 0.960784376f, 0.960784376f, 0.862745166f, 1.000000000f };
			static const NColor Bisque = { 1.000000000f, 0.894117713f, 0.768627524f, 1.000000000f };
			static const NColor Black = { 0.000000000f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const NColor BlanchedAlmond = { 1.000000000f, 0.921568692f, 0.803921640f, 1.000000000f };
			static const NColor Blue = { 0.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const NColor BlueViolet = { 0.541176498f, 0.168627456f, 0.886274576f, 1.000000000f };
			static const NColor Brown = { 0.647058845f, 0.164705887f, 0.164705887f, 1.000000000f };
			static const NColor BurlyWood = { 0.870588303f, 0.721568644f, 0.529411793f, 1.000000000f };
			static const NColor CadetBlue = { 0.372549027f, 0.619607866f, 0.627451003f, 1.000000000f };
			static const NColor Chartreuse = { 0.498039246f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const NColor Chocolate = { 0.823529482f, 0.411764741f, 0.117647067f, 1.000000000f };
			static const NColor Coral = { 1.000000000f, 0.498039246f, 0.313725501f, 1.000000000f };
			static const NColor CornflowerBlue = { 0.392156899f, 0.584313750f, 0.929411829f, 1.000000000f };
			static const NColor Cornsilk = { 1.000000000f, 0.972549081f, 0.862745166f, 1.000000000f };
			static const NColor Crimson = { 0.862745166f, 0.078431375f, 0.235294133f, 1.000000000f };
			static const NColor Cyan = { 0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const NColor DarkBlue = { 0.000000000f, 0.000000000f, 0.545098066f, 1.000000000f };
			static const NColor DarkCyan = { 0.000000000f, 0.545098066f, 0.545098066f, 1.000000000f };
			static const NColor DarkGoldenrod = { 0.721568644f, 0.525490224f, 0.043137256f, 1.000000000f };
			static const NColor DarkGray = { 0.662745118f, 0.662745118f, 0.662745118f, 1.000000000f };
			static const NColor DarkGreen = { 0.000000000f, 0.392156899f, 0.000000000f, 1.000000000f };
			static const NColor DarkKhaki = { 0.741176486f, 0.717647076f, 0.419607878f, 1.000000000f };
			static const NColor DarkMagenta = { 0.545098066f, 0.000000000f, 0.545098066f, 1.000000000f };
			static const NColor DarkOliveGreen = { 0.333333343f, 0.419607878f, 0.184313729f, 1.000000000f };
			static const NColor DarkOrange = { 1.000000000f, 0.549019635f, 0.000000000f, 1.000000000f };
			static const NColor DarkOrchid = { 0.600000024f, 0.196078449f, 0.800000072f, 1.000000000f };
			static const NColor DarkRed = { 0.545098066f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const NColor DarkSalmon = { 0.913725555f, 0.588235319f, 0.478431404f, 1.000000000f };
			static const NColor DarkSeaGreen = { 0.560784340f, 0.737254918f, 0.545098066f, 1.000000000f };
			static const NColor DarkSlateBlue = { 0.282352954f, 0.239215702f, 0.545098066f, 1.000000000f };
			static const NColor DarkSlateGray = { 0.184313729f, 0.309803933f, 0.309803933f, 1.000000000f };
			static const NColor DarkTurquoise = { 0.000000000f, 0.807843208f, 0.819607913f, 1.000000000f };
			static const NColor DarkViolet = { 0.580392182f, 0.000000000f, 0.827451050f, 1.000000000f };
			static const NColor DeepPink = { 1.000000000f, 0.078431375f, 0.576470613f, 1.000000000f };
			static const NColor DeepSkyBlue = { 0.000000000f, 0.749019623f, 1.000000000f, 1.000000000f };
			static const NColor DimGray = { 0.411764741f, 0.411764741f, 0.411764741f, 1.000000000f };
			static const NColor DodgerBlue = { 0.117647067f, 0.564705908f, 1.000000000f, 1.000000000f };
			static const NColor Firebrick = { 0.698039234f, 0.133333340f, 0.133333340f, 1.000000000f };
			static const NColor FloralWhite = { 1.000000000f, 0.980392218f, 0.941176534f, 1.000000000f };
			static const NColor ForestGreen = { 0.133333340f, 0.545098066f, 0.133333340f, 1.000000000f };
			static const NColor Fuchsia = { 1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const NColor Gainsboro = { 0.862745166f, 0.862745166f, 0.862745166f, 1.000000000f };
			static const NColor GhostWhite = { 0.972549081f, 0.972549081f, 1.000000000f, 1.000000000f };
			static const NColor Gold = { 1.000000000f, 0.843137324f, 0.000000000f, 1.000000000f };
			static const NColor Goldenrod = { 0.854902029f, 0.647058845f, 0.125490203f, 1.000000000f };
			static const NColor Gray = { 0.501960814f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const NColor Green = { 0.000000000f, 0.501960814f, 0.000000000f, 1.000000000f };
			static const NColor GreenYellow = { 0.678431392f, 1.000000000f, 0.184313729f, 1.000000000f };
			static const NColor Honeydew = { 0.941176534f, 1.000000000f, 0.941176534f, 1.000000000f };
			static const NColor HotPink = { 1.000000000f, 0.411764741f, 0.705882370f, 1.000000000f };
			static const NColor IndianRed = { 0.803921640f, 0.360784322f, 0.360784322f, 1.000000000f };
			static const NColor Indigo = { 0.294117659f, 0.000000000f, 0.509803951f, 1.000000000f };
			static const NColor Ivory = { 1.000000000f, 1.000000000f, 0.941176534f, 1.000000000f };
			static const NColor Khaki = { 0.941176534f, 0.901960850f, 0.549019635f, 1.000000000f };
			static const NColor Lavender = { 0.901960850f, 0.901960850f, 0.980392218f, 1.000000000f };
			static const NColor LavenderBlush = { 1.000000000f, 0.941176534f, 0.960784376f, 1.000000000f };
			static const NColor LawnGreen = { 0.486274540f, 0.988235354f, 0.000000000f, 1.000000000f };
			static const NColor LemonChiffon = { 1.000000000f, 0.980392218f, 0.803921640f, 1.000000000f };
			static const NColor LightBlue = { 0.678431392f, 0.847058892f, 0.901960850f, 1.000000000f };
			static const NColor LightCoral = { 0.941176534f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const NColor LightCyan = { 0.878431439f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const NColor LightGoldenrodYellow = { 0.980392218f, 0.980392218f, 0.823529482f, 1.000000000f };
			static const NColor LightGreen = { 0.564705908f, 0.933333397f, 0.564705908f, 1.000000000f };
			static const NColor LightGray = { 0.827451050f, 0.827451050f, 0.827451050f, 1.000000000f };
			static const NColor LightPink = { 1.000000000f, 0.713725507f, 0.756862819f, 1.000000000f };
			static const NColor LightSalmon = { 1.000000000f, 0.627451003f, 0.478431404f, 1.000000000f };
			static const NColor LightSeaGreen = { 0.125490203f, 0.698039234f, 0.666666687f, 1.000000000f };
			static const NColor LightSkyBlue = { 0.529411793f, 0.807843208f, 0.980392218f, 1.000000000f };
			static const NColor LightSlateGray = { 0.466666698f, 0.533333361f, 0.600000024f, 1.000000000f };
			static const NColor LightSteelBlue = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
			static const NColor LightYellow = { 1.000000000f, 1.000000000f, 0.878431439f, 1.000000000f };
			static const NColor Lime = { 0.000000000f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const NColor LimeGreen = { 0.196078449f, 0.803921640f, 0.196078449f, 1.000000000f };
			static const NColor Linen = { 0.980392218f, 0.941176534f, 0.901960850f, 1.000000000f };
			static const NColor Magenta = { 1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f };
			static const NColor Maroon = { 0.501960814f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const NColor MediumAquamarine = { 0.400000036f, 0.803921640f, 0.666666687f, 1.000000000f };
			static const NColor MediumBlue = { 0.000000000f, 0.000000000f, 0.803921640f, 1.000000000f };
			static const NColor MediumOrchid = { 0.729411781f, 0.333333343f, 0.827451050f, 1.000000000f };
			static const NColor MediumPurple = { 0.576470613f, 0.439215720f, 0.858823597f, 1.000000000f };
			static const NColor MediumSeaGreen = { 0.235294133f, 0.701960802f, 0.443137288f, 1.000000000f };
			static const NColor MediumSlateBlue = { 0.482352972f, 0.407843173f, 0.933333397f, 1.000000000f };
			static const NColor MediumSpringGreen = { 0.000000000f, 0.980392218f, 0.603921592f, 1.000000000f };
			static const NColor MediumTurquoise = { 0.282352954f, 0.819607913f, 0.800000072f, 1.000000000f };
			static const NColor MediumVioletRed = { 0.780392230f, 0.082352944f, 0.521568656f, 1.000000000f };
			static const NColor MidnightBlue = { 0.098039225f, 0.098039225f, 0.439215720f, 1.000000000f };
			static const NColor MintCream = { 0.960784376f, 1.000000000f, 0.980392218f, 1.000000000f };
			static const NColor MistyRose = { 1.000000000f, 0.894117713f, 0.882353008f, 1.000000000f };
			static const NColor Moccasin = { 1.000000000f, 0.894117713f, 0.709803939f, 1.000000000f };
			static const NColor NavajoWhite = { 1.000000000f, 0.870588303f, 0.678431392f, 1.000000000f };
			static const NColor Navy = { 0.000000000f, 0.000000000f, 0.501960814f, 1.000000000f };
			static const NColor OldLace = { 0.992156923f, 0.960784376f, 0.901960850f, 1.000000000f };
			static const NColor Olive = { 0.501960814f, 0.501960814f, 0.000000000f, 1.000000000f };
			static const NColor OliveDrab = { 0.419607878f, 0.556862772f, 0.137254909f, 1.000000000f };
			static const NColor Orange = { 1.000000000f, 0.647058845f, 0.000000000f, 1.000000000f };
			static const NColor OrangeRed = { 1.000000000f, 0.270588249f, 0.000000000f, 1.000000000f };
			static const NColor Orchid = { 0.854902029f, 0.439215720f, 0.839215755f, 1.000000000f };
			static const NColor PaleGoldenrod = { 0.933333397f, 0.909803987f, 0.666666687f, 1.000000000f };
			static const NColor PaleGreen = { 0.596078455f, 0.984313786f, 0.596078455f, 1.000000000f };
			static const NColor PaleTurquoise = { 0.686274529f, 0.933333397f, 0.933333397f, 1.000000000f };
			static const NColor PaleVioletRed = { 0.858823597f, 0.439215720f, 0.576470613f, 1.000000000f };
			static const NColor PapayaWhip = { 1.000000000f, 0.937254965f, 0.835294187f, 1.000000000f };
			static const NColor PeachPuff = { 1.000000000f, 0.854902029f, 0.725490212f, 1.000000000f };
			static const NColor Peru = { 0.803921640f, 0.521568656f, 0.247058839f, 1.000000000f };
			static const NColor Pink = { 1.000000000f, 0.752941251f, 0.796078503f, 1.000000000f };
			static const NColor Plum = { 0.866666734f, 0.627451003f, 0.866666734f, 1.000000000f };
			static const NColor PowderBlue = { 0.690196097f, 0.878431439f, 0.901960850f, 1.000000000f };
			static const NColor Purple = { 0.501960814f, 0.000000000f, 0.501960814f, 1.000000000f };
			static const NColor Red = { 1.000000000f, 0.000000000f, 0.000000000f, 1.000000000f };
			static const NColor RosyBrown = { 0.737254918f, 0.560784340f, 0.560784340f, 1.000000000f };
			static const NColor RoyalBlue = { 0.254901975f, 0.411764741f, 0.882353008f, 1.000000000f };
			static const NColor SaddleBrown = { 0.545098066f, 0.270588249f, 0.074509807f, 1.000000000f };
			static const NColor Salmon = { 0.980392218f, 0.501960814f, 0.447058856f, 1.000000000f };
			static const NColor SandyBrown = { 0.956862807f, 0.643137276f, 0.376470625f, 1.000000000f };
			static const NColor SeaGreen = { 0.180392161f, 0.545098066f, 0.341176480f, 1.000000000f };
			static const NColor SeaShell = { 1.000000000f, 0.960784376f, 0.933333397f, 1.000000000f };
			static const NColor Sienna = { 0.627451003f, 0.321568638f, 0.176470593f, 1.000000000f };
			static const NColor Silver = { 0.752941251f, 0.752941251f, 0.752941251f, 1.000000000f };
			static const NColor SkyBlue = { 0.529411793f, 0.807843208f, 0.921568692f, 1.000000000f };
			static const NColor SlateBlue = { 0.415686309f, 0.352941185f, 0.803921640f, 1.000000000f };
			static const NColor SlateGray = { 0.439215720f, 0.501960814f, 0.564705908f, 1.000000000f };
			static const NColor Snow = { 1.000000000f, 0.980392218f, 0.980392218f, 1.000000000f };
			static const NColor SpringGreen = { 0.000000000f, 1.000000000f, 0.498039246f, 1.000000000f };
			static const NColor SteelBlue = { 0.274509817f, 0.509803951f, 0.705882370f, 1.000000000f };
			static const NColor Tan = { 0.823529482f, 0.705882370f, 0.549019635f, 1.000000000f };
			static const NColor Teal = { 0.000000000f, 0.501960814f, 0.501960814f, 1.000000000f };
			static const NColor Thistle = { 0.847058892f, 0.749019623f, 0.847058892f, 1.000000000f };
			static const NColor Tomato = { 1.000000000f, 0.388235331f, 0.278431386f, 1.000000000f };
			static const NColor Transparent = { 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f };
			static const NColor Turquoise = { 0.250980407f, 0.878431439f, 0.815686345f, 1.000000000f };
			static const NColor Violet = { 0.933333397f, 0.509803951f, 0.933333397f, 1.000000000f };
			static const NColor Wheat = { 0.960784376f, 0.870588303f, 0.701960802f, 1.000000000f };
			static const NColor White = { 1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
			static const NColor WhiteSmoke = { 0.960784376f, 0.960784376f, 0.960784376f, 1.000000000f };
			static const NColor Yellow = { 1.000000000f, 1.000000000f, 0.000000000f, 1.000000000f };
			static const NColor YellowGreen = { 0.603921592f, 0.803921640f, 0.196078449f, 1.000000000f };
		}
	}
}

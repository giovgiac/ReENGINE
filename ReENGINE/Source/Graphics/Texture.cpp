/*
 * Texture.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Texture.hpp"

#include "String/Character.hpp"

#include <FreeImage.h>

const u32 defaultBPP = 32;
const u32 defaultWidth = 2048;
const u32 defaultHeight = 2048;

u8* buildDefaultImage()
{
    FIBITMAP* dib = FreeImage_Allocate(defaultWidth, defaultHeight, defaultBPP, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);

    // Calculate the number of bytes per pixel.
    i32 bpp = FreeImage_GetLine(dib) / FreeImage_GetWidth(dib);

    // Go through image pixels to build default grid pattern.
    for (u32 y = 0; y < defaultHeight; ++y)
    {
        u8* bytes = FreeImage_GetScanLine(dib, y);
        for (u32 x = 0; x < defaultWidth; ++x)
        {
            // Set pixel color to grid pattern.
            if (((x < defaultWidth / 2) && (y < defaultHeight / 2)) || ((x > defaultWidth / 2) && (y > defaultHeight / 2)))
            {
                bytes[FI_RGBA_RED] = 63;
                bytes[FI_RGBA_GREEN] = 63;
                bytes[FI_RGBA_BLUE] = 63;
                bytes[FI_RGBA_ALPHA] = 255;
            }
            else
            {
                bytes[FI_RGBA_RED] = 255;
                bytes[FI_RGBA_GREEN] = 255;
                bytes[FI_RGBA_BLUE] = 255;
                bytes[FI_RGBA_ALPHA] = 255;
            }

            // Jump to next pixel.
            bytes += bpp;
        }
    }

    // Return pixels.
    return FreeImage_GetBits(dib);
}

static u8* defaultImage = buildDefaultImage();

namespace Re
{
    namespace Graphics
    {
        Texture::Texture()
            : _filename(""), _isLoaded(false), _width(0), _height(0), _pixels(), _handle(nullptr)
        {}

        Texture::Texture(const utf8* filename)
            : Texture()
        {
            // Set values as specified.
            // TODO: Revert back to const utf8* and use StrDup.
            _filename = filename;
        }

        Texture::~Texture()
        {}

        void Texture::Load()
        {
            if (_isLoaded) return;
            if (_filename == "")
            {
                LoadDefaultTexture();
                return;
            }
            else
            {
                FREE_IMAGE_FORMAT format = FreeImage_GetFileType(_filename.c_str());

                if (format != FIF_UNKNOWN)
                {
                    FIBITMAP* raw = FreeImage_Load(format, _filename.c_str());
                    FIBITMAP* img = FreeImage_ConvertTo32Bits(raw);

                    if (img)
                    {
                        FreeImage_Unload(raw);

                        // Save image information into texture.
                        _bpp = FreeImage_GetBPP(img) / 8;
                        _width = FreeImage_GetWidth(img);
                        _height = FreeImage_GetHeight(img);
                        _pixels = FreeImage_GetBits(img);
                        _handle = img;
                        _isLoaded = true;
                        return;
                    }
                }
            }

            Core::Debug::Error("Error while loading texture at %s.", _filename);
            LoadDefaultTexture();
        }

        void Texture::Unload()
        {
            if (!_isLoaded) return;

            FIBITMAP* img = static_cast<FIBITMAP*>(_handle);

            if (img)
            {
                FreeImage_Unload(img);
            }

            // Reset texture data to nothingness, except filename.
            _bpp = 0;
            _width = 0;
            _height = 0;
            _pixels = nullptr;
            _handle = nullptr;
            _isLoaded = false;
        }

        bool Texture::IsLoaded() const
        {
            return _isLoaded;
        }

        usize Texture::GetWidth() const
        {
            return static_cast<usize>(_width);
        }

        usize Texture::GetHeight() const
        {
            return static_cast<usize>(_height);
        }

        usize Texture::GetBPP() const
        {
            return static_cast<usize>(_bpp);
        }

        u8* Texture::GetPixels() const
        {
            return _pixels;
        }

        void Texture::LoadDefaultTexture()
        {
            // Save default information (grid pattern) into texture.
            _bpp = defaultBPP / 8;
            _width = defaultWidth;
            _height = defaultHeight;
            _pixels = defaultImage;
            _handle = nullptr;
            _isLoaded = true;
        }
    }
}

/*
 * Texture.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Texture.hpp"

#include <FreeImage.h>

const u32 defaultWidth = 32;
const u32 defaultHeight = 32;

static boost::container::vector<u8> buildDefaultImage()
{
    boost::container::vector<u8> result;

    // Resize array of pixels appropriately.
    result.resize(defaultWidth * defaultHeight * 4);

    // Build the default grid pattern.
    for (usize i = 0; i < defaultHeight; ++i)
    {
        for (usize j = 0; j < defaultWidth; ++j)
        {
            if (i % 2 == 0)
            {
                result[defaultWidth * defaultHeight * 0 + defaultWidth * j + i] = 255;
                result[defaultWidth * defaultHeight * 1 + defaultWidth * j + i] = 255;
                result[defaultWidth * defaultHeight * 2 + defaultWidth * j + i] = 255;
                result[defaultWidth * defaultHeight * 3 + defaultWidth * j + i] = 255;
            }
            else
            {
                result[defaultWidth * defaultHeight * 0 + defaultWidth * j + i] = 63;
                result[defaultWidth * defaultHeight * 1 + defaultWidth * j + i] = 63;
                result[defaultWidth * defaultHeight * 2 + defaultWidth * j + i] = 63;
                result[defaultWidth * defaultHeight * 3 + defaultWidth * j + i] = 255;
            }
        }
    }

    return result;
}

const boost::container::vector<u8> defaultImage = buildDefaultImage();

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
            _filename = filename;
        }

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
                FREE_IMAGE_FORMAT format = FreeImage_GetFileType(_filename);

                if (format != FIF_UNKNOWN)
                {
                    FIBITMAP* raw = FreeImage_Load(format, _filename);
                    FIBITMAP* img = FreeImage_ConvertTo32Bits(raw);

                    if (img)
                    {
                        FreeImage_Unload(raw);

                        // Save image information into texture.
                        u8* bytes = FreeImage_GetBits(img);
                        _width = FreeImage_GetWidth(img);
                        _height = FreeImage_GetHeight(img);
                        _pixels.assign(bytes, bytes + (static_cast<usize>(_width) * static_cast<usize>(_height) * 4));
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
            _width = 0;
            _height = 0;
            _pixels = {};
            _handle = nullptr;
            _isLoaded = false;
        }

        bool Texture::IsLoaded() const
        {
            return _isLoaded;
        }

        u32 Texture::GetWidth() const
        {
            return _width;
        }

        u32 Texture::GetHeight() const
        {
            return _height;
        }

        boost::container::vector<u8> Texture::GetPixels() const
        {
            return _pixels;
        }

        void Texture::LoadDefaultTexture()
        {
            // Save default information (grid pattern) into texture.
            _width = defaultWidth;
            _height = defaultHeight;
            _pixels = defaultImage;
            _handle = nullptr;
            _isLoaded = true;
        }
    }
}

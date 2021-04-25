/*
 * Texture.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re
{
    namespace Graphics
    {
        class Texture
        {
        public:
            Texture();
            explicit Texture(const utf8* filename);

            void Load();
            void Unload();

            bool IsLoaded() const;
            usize GetWidth() const;
            usize GetHeight() const;
            usize GetBPP() const;
            u8* GetPixels() const;

        private:
            void LoadDefaultTexture();

        private:
            const utf8* _filename;
            bool _isLoaded;

            // Stores the amount of bytes per pixel (usually 4).
            u32 _bpp;

            u32 _width;
            u32 _height;
            u8* _pixels;
            
            void* _handle;

        };
    }
}

/*
 * Texture.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

#include <boost/container/vector.hpp>

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
            u32 GetWidth() const;
            u32 GetHeight() const;
            boost::container::vector<u8> GetPixels() const;

        private:
            void LoadDefaultTexture();

        private:
            const utf8* _filename;
            bool _isLoaded;

            u32 _width;
            u32 _height;
            boost::container::vector<u8> _pixels;
            
            void* _handle;

        };
    }
}

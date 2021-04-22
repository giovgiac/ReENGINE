/*
 * Debug.hpp
 *
 * This header file declares the Debug class which is
 * responsible for logging and printing warning and error messages
 * for the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Platform/HAL.hpp"

#define DEBUG_BUFFER 512

namespace Re
{
    namespace Core
    {
        /* 
         * @brief This class is responsible for managing Debug Printing and
         * Logging throughout the ReENGINE. 
         *
         */
        class Debug
        {
        public:
            /**
             * @brief This method outputs a Unicode String to the standard output.
             *
             * @param pStr: the pointer to the unformatted version of the string to output.
             * @param args: the arguments to fill the string's parameters.
             *
             */
            template <typename... ParamType, u32 bufferSize = DEBUG_BUFFER>
            static void Log(utf8* pStr, ParamType ... args)
            {
                // Create Buffer
                utf8 pBuffer[bufferSize];

                // Write to Buffer and Output
                snprintf(pBuffer, bufferSize, pStr, args...);
                OutputDebugString(pBuffer);
            }

            /**
             * ...
             */
            template <typename... ParamType, u32 bufferSize = DEBUG_BUFFER>
            static void Warning(utf8*, ParamType ...)
            {
            }

            /**
             * @brief This method outputs a Unicode String to a Windows MessageBox.
             *
             * @param pStr: the pointer to the unformatted version of the string to output.
             * @param args: the arguments to fill the string's parameters.
             *
             */
            template <typename... ParamType, u32 bufferSize = DEBUG_BUFFER>
            static void Error(utf8* pStr, ParamType ... args)
            {
                // Create Buffer
                utf8 pBuffer[bufferSize];

                // Write to Buffer and Create MessageBox
                snprintf(pBuffer, bufferSize, pStr, args...);
                MessageBox(nullptr, pBuffer, NTEXT("ReENGINE Error"), MB_OK);
            }
        };
    }
}

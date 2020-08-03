/*
 * MemoryManager.hpp
 *
 * This header file declares the MemoryManager class, responsible for
 * handling memory allocation and freeing throughout the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Manager.hpp"

// Allocator Includes
#include "Memory/DefaultAllocator.hpp"
#include "Memory/StackAllocator.hpp"

namespace Re 
{
	namespace Memory 
	{
		/* 
		 * @brief This class is responsible for managing memory throughout
		 * the other ReENGINE's subsystems. 
		 *
		 */
		class MemoryManager : public Core::Manager 
		{
		public:
			/* 
			 * @brief This constructor does nothing, since the method
			 * StartUp is supposed to be used. 
			 *
			 */
			MemoryManager() {}

			/* 
			 * @brief This destructor does nothing, since the method
			 * ShutDown is supposed to be used. 
			 *
			 */
			~MemoryManager() {}

			/* 
			 * @brief This method starts up the Memory Management system,
			 * allocating memory for the global stack, among other things.
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			NRESULT StartUp() override;

			/* 
			 * @brief This method shuts down the Memory Management system,
			 * releasing the memory of the global stack, among other things.
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			NRESULT ShutDown() override;
		};
	}
}

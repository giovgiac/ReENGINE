/*
 * NewtonManager.hpp
 * 
 * This header file declares the most important class in the
 * ReENGINE: the module start-up and shut-down, responsible for starting
 * and terminating all of the engine's modules.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Localization/LocalizationManager.hpp"
#include "Memory/MemoryManager.hpp"
// TO BE INCLUDED: Subsystem Managers

namespace Re
{
	namespace Core
	{
		/* 
		 * @brief This class is responsible for managing all of the
		 * ReENGINE's subsystems. 
		 *
		 */
		class NewtonManager : public Manager 
		{
		private:
			Localization::LocalizationManager _nLocalizationManager;
			Memory::MemoryManager _nMemoryManager;
			// TO BE ADDED: Subsystem Managers

		public:
			/* 
			 * @brief This constructor does nothing, since the method
			 * to be used is StartUp. 
			 *
			 */
			NewtonManager() {}

			/* 
			 * @brief This destructor does nothing, since the method
			 * to be used is ShutDown.
			 *
			 */
			~NewtonManager() {}

			/* 
			 * @brief This method starts up all the engine subsystems
			 * defined throughout the project in the correct order.
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			NRESULT StartUp() override;

			/* 
			 * @brief This method shuts down all the engine subsystems
			 * defined throughout the project in the reverse order.
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			NRESULT ShutDown() override;
		};
	}
}

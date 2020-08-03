/*
 * NewtonManager.cpp
 *
 * This source file defines the functions declared in the
 * NewtonManager.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "NewtonManager.hpp"

namespace Re {
	namespace Core {
		NRESULT NewtonManager::StartUp() {
			_nMemoryManager.StartUp();
			_nLocalizationManager.StartUp();
			// TO BE IMPLEMENTED: Start-up individual subsystems

			return NSUCCESS;
		}

		NRESULT NewtonManager::ShutDown() {
			// TO BE IMPLEMENTED: Shut-down individual subsystems
			_nLocalizationManager.ShutDown();
			_nMemoryManager.ShutDown();

			return NSUCCESS;
		}
	}
}

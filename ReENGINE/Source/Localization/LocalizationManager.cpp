/*
 * LocalizationManager.cpp
 *
 * This source file implements the methods declared in the
 * LocalizationManager.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "LocalizationManager.hpp"

namespace Re {
	namespace Core {
		namespace Localization {
			NRESULT LocalizationManager::StartUp() {
				return NSUCCESS;
			}

			NRESULT LocalizationManager::ShutDown() {
				return NSUCCESS;
			}

			utf8* LocalizationManager::GetLocalizedString(utf8* id) {
				return nullptr;
			}
		}
	}
}

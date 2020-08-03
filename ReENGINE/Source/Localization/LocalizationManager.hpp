/*
 * LocalizationManager.hpp
 *
 * This header file declares the LocalizationManager, the class responsible
 * for dealing with differences in language and whatnot in the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Manager.hpp"

#define LANGUAGE_CATE NTEXT("Language")
#define LANGUAGE_FILE NTEXT("language.nml")

namespace Re {
	namespace Core {
		namespace Localization {
			/* 
			 * LocalizationManager Class
			 *
			 * This class is responsible for handling all of the Localization tasks
			 * associated with the ReENGINE. 
			 *
			 */
			class LocalizationManager : public Manager {
			private:
			public:
				/* 
				 * LocalizationManager Constructor
				 *
				 * This constructor does nothing, since the method to be
				 * used is StartUp.
				 *
				 */
				LocalizationManager() {}
				
				/* 
				 * LocalizationManager Destructor
				 *
				 * This destructor does nothing, since the method to be
				 * used is ShutDown. 
				 *
				 */
				~LocalizationManager() {}

				/* 
				 * LocalizationManager StartUp Method
				 *
				 * This method is responsible for starting up the LocalizationManager,
				 * creating the NMap and loading the strings into it.
				 *
				 * return: NSUCCESS if successful, NFAILURE otherwise.
				 *
				 */
				NRESULT StartUp() override;

				/* 
				 * LocalizationManager ShutDown Method
				 *
				 * This method is responsible for terminating the LocalizationManager,
				 * releasing the memory from the NMap.
				 *
				 * return: NSUCCESS if successful, NFAILURE otherwise. 
				 *
				 */
				NRESULT ShutDown() override;

				/* 
				 * LocalizationManager GetLocalizedString StaticMethod
				 *
				 * This method takes in a string ID and gives back a translated
				 * string.
				 *
				 * utf8* id: the pointer to the string ID.
				 *
				 * return: the string in the language representing the ID. 
				 *
				 */
				static utf8* GetLocalizedString(utf8* id);
			};
		}
	}
}

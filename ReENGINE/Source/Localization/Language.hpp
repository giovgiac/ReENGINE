/*
 * Language.hpp
 *
 * ...
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "String/Character.hpp"

// Language Configurations
#define LANGUAGE_CATEGORY	NTEXT("Language")

namespace Re {
	namespace Core {
		namespace Localization {
			/*
			 * NLANGUAGE Enum
			 *
			 * ...
			 *
			 */
			enum NLANGUAGE {
				en_US,
				pt_BR
			};

			/*
			 * ReadLanguage Function
			 *
			 * ...
			 *
			 * return: an NRESULT with the code representing success or failure.
			 *
			 */
			extern NRESULT ReadLanguage(utf8* filename, NLANGUAGE language);
		}
	}
}

/*
 * Manager.hpp
 *
 * This header file declares and defines a purely virtual class which
 * is inherited by the Manager classes in the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re 
{
	namespace Core 
	{
		/* 
		 * @brief This is a virtual class which is inherited by all the Manager
		 * classes in the ReENGINE. 
		 *
		 */
		class Manager 
		{
		public:
			/* 
			 * @brief This constructor does nothing, since the method to be
			 * used is the StartUp method. 
			 *
			 */
			Manager() {}

			/* 
			 * @brief This destructor does nothing, since the method to be
			 * used is the ShutDown method. 
			 *
			 */
			virtual ~Manager() {}

			/* 
			 * @brief This method is a purely virtual method, supposed to be
			 * overwritten by subclasses and it initializes the manager's
			 * systems.
			 *
			 * @return: NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			virtual NRESULT StartUp() = 0;

			/* 
			 * @brief This method is a purely virtual method, supposed to be
			 * overwritten by subclasses and it shuts down the manager's
			 * systems.
			 *
			 * @return: NSUCCESS if successful, NFAILURE otherwise.
			 *
			 */
			virtual NRESULT ShutDown() = 0;
		};
	}
}

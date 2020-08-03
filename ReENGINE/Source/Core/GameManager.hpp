/*
 * GameManager.hpp
 *
 * This header file declares the GameManager, the class that holds the Core information
 * about the game created with the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Manager.hpp"
#include "Platform/Timer.hpp"

namespace Re 
{
	namespace Core 
	{
		/* 
		 * @brief This class represents the Game, holding it's main functions for
		 * starting, drawing, updating and etc. 
		 *
		 */
		class GameManager : public Manager
		{
		public:
			/* 
			 * @brief This constructor does nothing, since the method to be 
			 * used is the StartUp method. 
			 *
			 */
			GameManager() {}

			/* 
			 * @brief This destructor does nothing, since the method to be 
			 * used is the ShutDown method. 
			 *
			 */
			~GameManager() {}

			/* 
			 * @brief This method is a purely virtual method that draws, especially
			 * graphics to the Video HID.
			 *
			 * @param timer: a reference to the game timer. 
			 *
			 */
			virtual void Draw(const Platform::ITimer& timer) = 0;

			/* 
			 * @brief This method is used to handle Keyboard Input and is called everytime
			 * a key is passed down. The WPARAM paramater contains the key used.
			 *
			 * @param WPARAM: the wParam that contains the key used. 
			 *
			 */
			virtual void OnKeyDown(WPARAM) {}

			/* 
			 * @brief This method is used to handle Keyboard Input and is called everytime
			 * a key is released. The WPARAM parameter contains the key released.
			 *
			 * @param WPARAM: the wParam that contains the key released.
			 *
			 */
			virtual void OnKeyUp(WPARAM) {}

			/* 
			 * @brief This method is used to handle Mouse Input and is called everytime
			 * a mouse button is down. The WPARAM parameter contains the button in
			 * question, while the NINTs contain x and y coordinates, respectively.
			 *
			 * @param WPARAM: the wParam that contains the button used.
			 * @param i32: the x-coordinate of the mouse at the time.
			 * @param i32: the y-coordinate of the mouse at the time.
			 *
			 */
			virtual void OnMouseDown(WPARAM, i32, i32) {}

			/* 
			 * @brief This method is used to handle Mouse Input and is called everytime
			 * the mouse is moved. The WPARAM parameter contains information about
			 * the mouse, while the NINTs contain x and y coordinates, respectively.
			 *
			 * @param WPARAM: the wParam that contains information about the mouse.
			 * @param i32: the x-coordinate of the mouse.
			 * @param i32: the y-coordinate of the mouse. 
			 *
			 */
			virtual void OnMouseMove(WPARAM, i32, i32) {}
			
			/* 
			 * @brief This method is used to handle Mouse Input and is called everytime
			 * a mouse button is up. The WPARAM parameter contains the button in
			 * question, while the NINTs contain x and y coordinates, respectively.
			 *
			 * @param WPARAM: the wParam that contains the button released.
			 * @param i32: the x-coordinate of the mouse at the time.
			 * @param i32: the y-coordinate of the mouse at the time.
			 *
			 */
			virtual void OnMouseUp(WPARAM, i32, i32) {}

			/* 
			 * @brief This method is a purely virtual method, supposed to be 
			 * overwritten by subclasses and it initializes the GameManager's
			 * subsystems.
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			virtual NRESULT StartUp() = 0;

			/* 
			 * @brief This method is a purely virtual method, supposed to be
			 * overwritten by subclasses and it shuts down the GameManager's
			 * subsystems. 
			 *
			 * @return NSUCCESS if successful, NFAILURE otherwise. 
			 *
			 */
			virtual NRESULT ShutDown() = 0;

			/* 
			 * @brief This method is a purely virtual method that updates the
			 * Game, especially graphics, every frame.
			 *
			 * @param timer: a reference to the game timer.
			 *
			 */
			virtual void Update(const Platform::ITimer& timer) = 0;
		};
	}
}

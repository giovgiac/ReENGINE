/*
 * Window.hpp
 * 
 * This header file declares a class responsible for drawing windows on Win32 system
 * and handling their messages.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Core/Input.hpp"

#include <boost/signals2.hpp>

#define RELEASE 0
#define PRESS 1

namespace Re
{
	namespace Platform
	{
		enum class WindowResult
		{
			Success = 0,
			Failure = 1
		};

		/*
		 * @brief This class is responsible for creating objects that represent
		 * and/or abstract Win32 windows. It is capable of creating them
		 * and also sending and handling messages.
		 *
		 */
		class Win32Window 
		{
		private:
			HWND		_hWnd;
			HINSTANCE	_hInstance;
			LPCSTR		_title;
			i32			_width;
			i32			_height;
			bool		_shouldClose;

			i32			_prevMouseX;
			i32			_prevMouseY;
			bool		_firstMouse;

		public:
			boost::signals2::signal<void(Core::Input::Action, Core::Input::Keys)> KeyEvent;
			boost::signals2::signal<void(i32, i32)> MouseEvent;

		public:
			/*
			 * @brief This constructor initializes the object's parameters to the
			 * passed in values.
			 * 
			 */
			Win32Window();

			~Win32Window();

			/*
			 * @brief This method registers and initializes a window with Windows.
			 *
			 * @param title: the title to be on the window's bar.
			 * @param width: the desired width for the window.
			 * @param height: the desired height for the window.
			 * @param nCmdShow: the code that represents the initial state of the window.
			 *
			 * @return true if window is created successfully, false otherwise.
			 *
			 */
			WindowResult Startup(LPCSTR title, i32 width, i32 height, i32 nCmdShow, bool captureMouse = true);

			void Shutdown();

			/*
			 * @brief This method is a getter for whether this window should close.
			 *
			 * @return if the window should close or not.
			 *
			 */
			INLINE bool GetShouldClose() const { return _shouldClose; }

			/*
			 * @brief This method is a getter for the hWnd parameter.
			 *
			 * @return the handler to the window object.
			 *
			 */
			INLINE HWND GetHandle() const { return _hWnd; }

			/*
			 * @brief This method is a getter for the hInstance parameter.
			 *
			 * @return the instance of the window object.
			 *
			 */
			INLINE HINSTANCE GetInstance() const { return _hInstance; }

			/*
			 * @brief This method is a getter for the width parameter.
			 *
			 * @return the width of the window object.
			 *
			 */
			INLINE i32 GetWidth() const { return _width; }

			/*
			 * @brief This method is a getter for the height parameter.
			 *
			 * @return the height of the window object.
			 *
			 */
			INLINE i32 GetHeight() const { return _height; }

			/*
			 * @brief This method is responsible for handling the idle state of the
			 * window object, receiving and sending messages.
			 *
			 */
			virtual void PollEvents();

			/*
			 * @brief This callback function is responsible for handling the Win32Window
			 * object's messages. It is called by the MainWndProc Callback and
			 * effectively handles messages sent by the MessageLoop Method.
			 *
			 * @param hWnd: the handle of the window.
			 * @param msg: the message received.
			 * @param wParam: the wParam that holds information about the message.
			 * @param lParam: the lParam that holds information about the message.
			 *
			 * @return either 0 or value of DefWindowProc callback.
			 *
			 */
			virtual u64 HandleEvents(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);

		};
	}
}

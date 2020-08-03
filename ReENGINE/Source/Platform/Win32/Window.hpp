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

#include "Core/GameManager.hpp"

namespace Re {
	/* 
	 * Win32Window Class
	 *
	 * This class is responsible for creating objects that represent
	 * and/or abstract Win32 windows. It is capable of creating them
	 * and also sending and handling messages. 
	 *
	 */
	class Win32Window {
	protected:
		HWND		_hWnd;
		HINSTANCE	_hInstance;
		LPCSTR		_title;
		i32			_width;
		i32			_height;
		bool		_resizing = false;
		bool		_windowed;

	public:
		/* 
		 * Win32Window Constructor
		 *
		 * This constructor initializes the object's parameters to the
		 * passed in values and also sets the mainWnd pointer to point
		 * to this window object. 
		 *
		 * HINSTANCE hInstance: the instance of the main application.
		 * LPCSTR title: the title to be on the window's bar.
		 * i32 width: the desired width for the window.
		 * i32 height: the desired height for the window.
		 * bool windowed: whether or not the window should be windowed.
		 *
		 */
		Win32Window(HINSTANCE hInstance, LPCSTR title, i32 width, i32 height, bool windowed = true);

		/* 
		 * Win32Window CreateDisplay Method
		 *
		 * This method registers and initializes a window with Windows.
		 *
		 * i32 nCmdShow: the code that represents the initial state of the window.
		 *
		 * return: true if window is created successfully, false otherwise. 
		 *
		 */
		NRESULT CreateDisplay(i32 nCmdShow);

		/* 
		 * Win32Window GetHandler Method
		 * 
		 * This method is a getter for the _hWnd parameter.
		 *
		 * return: the handler to the window object. 
		 *
		 */
		INLINE HWND GetHandle() { return _hWnd; }

		/* 
		 * Win32Window GetInstance Method
		 *
		 * This method is a getter for the _hInstance parameter.
		 *
		 * return: the instance of the window object. 
		 *
		 */
		INLINE HINSTANCE GetInstance() { return _hInstance; }

		/* 
		 * Win32Window GetTitle Method
		 *
		 * This method is a getter for the _title parameter.
		 *
		 * return: the title of the window object. 
		 *
		 */
		INLINE LPCSTR GetTitle() { return _title; }

		/* 
		 * Win32Window GetWidth Method
		 *
		 * This method is a getter for the _width parameter.
		 *
		 * return: the width of the window object. 
		 *
		 */
		INLINE u32 GetWidth() { return _width; }

		/* 
		 * Win32Window GetHeight Method
		 *
		 * This method is a getter for the _height parameter.
		 *
		 * return: the height of the window object. 
		 *
		 */
		INLINE u32 GetHeight() { return _height; }

		/* 
		 * Win32Window GetWindowed Method
		 *
		 * This method is a getter for the _windowed parameter.
		 *
		 * return: true if in windowed mode, false if fullscreen. 
		 *
		 */
		INLINE bool GetWindowed() { return _windowed; }

		/* 
		 * Win32Window SetWindowed Method
		 *
		 * This method is a setter for the _windowed parameter.
		 *
		 * const bool value: whether or not the window should be windowed.
		 *
		 * return: nothing. 
		 *
		 */
		INLINE void SetWindowed(const bool value) { _windowed = value; }

		/* 
		 * Win32Window MessageLoop Method
		 *
		 * This virtual method is responsible for handling the idle state of the
		 * window object, sending messages and initializing the main game
		 * engine loop.
		 *
		 * Core::GameManager* game: the pointer to the game object.
		 *
		 * return: an integer representing the wParam parameter of the message. 
		 *
		 */
		virtual i32 MessageLoop(Core::GameManager* game) = 0;

		/* 
		 * Win32Window DisplayProcedure Callback
		 *
		 * This callback function is responsible for handling the Win32Window
		 * object's messages. It is called by the MainWndProc Callback and
		 * effectively handles messages sent by the MessageLoop Method.
		 *
		 * HWND hWnd: the handle of the window.
		 * u32 msg: the message received.
		 * WPARAM wParam: the wParam that holds information about the message.
		 * LPARAM lParam: the lParam that holds information about the message.
		 *
		 * return: either 0 or value of DefWindowProc callback. 
		 *
		 */
		virtual LRESULT DisplayProcedure(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);
	};
}

// Externs the pointer to the Main Win32Window object.
extern Re::Win32Window* pMainWindow;

/* 
 * mainWnd MainWndProc Callback
 *
 * This callback function handles messages sent to the mainWnd.
 * It acts as a middle-man, sending messages to the mainWnd's object
 * personal handler.
 *
 * HWND hWnd: the handle of the window.
 * u32 msg: the message received.
 * WPARAM wParam: the wParam that holds information about the message.
 * LPARAM lParam: the lParam that holds information about the message.
 *
 * return: value of the DisplayProcedure callback. 
 *
 */
LRESULT CALLBACK MainWndProc(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);

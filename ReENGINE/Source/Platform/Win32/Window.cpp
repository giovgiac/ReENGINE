/*
 * Window.cpp
 *
 * This source file defines the class responsible for drawing windows on Win32 system
 * and handling their messages declared in the Window.hpp.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Window.hpp"

// Defaults the main window pointer to NULL.
Re::Win32Window* pMainWindow = nullptr;

LRESULT CALLBACK MainWndProc(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam) {
	// Set the Message Handler to be the Main Win32Window
	if (pMainWindow)
		return pMainWindow->DisplayProcedure(hWnd, msg, wParam, lParam);
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

namespace Re {
	Win32Window::Win32Window(HINSTANCE hInstance, LPCSTR title, i32 width, i32 height, bool windowed)
		: _hInstance(hInstance), _title(title), _width(width), _height(height), _windowed(windowed)
	{
		// Sets Current Object to be Main Win32Window
		pMainWindow = this;
	}

	NRESULT Win32Window::CreateDisplay(i32 nCmdShow) {
		// Create Win32Window Class
		WNDCLASS wc;

		// Configure Win32Window Properties
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInstance;
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wc.lpszClassName = NTEXT("NewtonWindow");
		wc.lpszMenuName = 0;
			
		// Register Window with Windows
		if (!RegisterClass(&wc)) {
			MessageBox(0, NTEXT("Failure to register window class with Windows!"), NTEXT("NEWTON ERROR"), MB_OK);
			return NFAILURE;
		}

		// Create Window
		_hWnd = CreateWindow(NTEXT("NewtonWindow"), _title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			_width, _height, 0, 0, _hInstance, 0);
		
		if (!_hWnd) {
			MessageBox(0, NTEXT("Failure to create window with Windows!"), NTEXT("NEWTON ERROR"), MB_OK);
			return NFAILURE;
		}

		// Show and Update Window
		ShowWindow(_hWnd, nCmdShow);
		UpdateWindow(_hWnd);

		return NSUCCESS;
	}

	LRESULT Win32Window::DisplayProcedure(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam) {
		// Handle Incoming Messages
		switch (msg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		// Unhandled Messages Forwarded to Default Handler
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

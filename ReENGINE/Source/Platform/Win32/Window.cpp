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

#include <boost/container/map.hpp>

boost::container::map<HWND, Re::Platform::Win32Window*> windowMap;

/*
 * @brief This callback function handles messages sent to the mainWnd.
 * It acts as a middle-man, sending messages to the mainWnd's object
 * personal handler.
 *
 * @param hWnd: the handle of the window.
 * @param msg: the message received.
 * @param wParam: the wParam that holds information about the message.
 * @param lParam: the lParam that holds information about the message.
 *
 * @return value of the HandleEvents callback.
 *
 */
LRESULT CALLBACK MainWndProc(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam)
{
	// Invoke the display procedure in the appropriate window.
	if (windowMap.find(hWnd) != windowMap.end())
	{
		return windowMap[hWnd]->HandleEvents(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

namespace Re
{
	namespace Platform
	{
		Win32Window::Win32Window()
			: _hWnd(NULL), _hInstance(NULL), _title(""), _width(-1), _height(-1), _shouldClose(false)
		{}

		WindowResult Win32Window::Startup(LPCSTR title, i32 width, i32 height, i32 nCmdShow)
		{
			// Store class members.
			_hInstance = GetModuleHandle(NULL);
			_title = title;
			_width = width;
			_height = height;

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
			wc.lpszClassName = NTEXT("ReENGINE");
			wc.lpszMenuName = 0;

			// Register Window with Windows
			if (!RegisterClass(&wc))
			{
				MessageBox(0, NTEXT("Failure to register window class with Windows!"), NTEXT("ReENGINE Error"), MB_OK);
				return WindowResult::Failure;
			}

			// Create Window
			_hWnd = CreateWindow(NTEXT("ReENGINE"), title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
								 width, height, 0, 0, _hInstance, 0);

			if (!_hWnd)
			{
				MessageBox(0, NTEXT("Failure to create window with Windows!"), NTEXT("ReEENGINE Error"), MB_OK);
				return WindowResult::Failure;
			}

			// Store window in map.
			windowMap.emplace(_hWnd, this);

			// Show and Update Window
			ShowWindow(_hWnd, nCmdShow);
			UpdateWindow(_hWnd);

			return WindowResult::Success;
		}

		void Win32Window::Shutdown()
		{
			DestroyWindow(_hWnd);
			UnregisterClass(NTEXT("ReENGINE"), _hInstance);
		}

		void Win32Window::PollEvents()
		{
			MSG msg = {};
			if (PeekMessage(&msg, _hWnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		u64 Win32Window::HandleEvents(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam)
		{
			// Handle incoming messages to this window.
			switch (msg)
			{
			case WM_KEYUP:
				if (wParam == VK_ESCAPE)
				{
					_shouldClose = true;
					PostQuitMessage(0);
					return 0;
				}

				break;
			case WM_DESTROY:
				_shouldClose = true;
				PostQuitMessage(0);
				return 0;
			}

			// Unhandled messages forwarded to the default handler.
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
}

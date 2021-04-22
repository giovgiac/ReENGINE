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
boost::container::map<i32, Re::Core::Input::Keys> codeToKey = {
	{ 0x41, Re::Core::Input::Keys::A },
	{ 0x44, Re::Core::Input::Keys::D },
	{ 0x53, Re::Core::Input::Keys::S },
	{ 0x57, Re::Core::Input::Keys::W }
};

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
			: _hWnd(NULL), _hInstance(NULL), _title(""), _width(-1), _height(-1), _shouldClose(false),
			_prevMouseX(0), _prevMouseY(0), _firstMouse(true)
		{}

		Win32Window::~Win32Window()
		{
			// Clear listening events upon window destruction.
			KeyEvent.disconnect_all_slots();
			MouseEvent.disconnect_all_slots();
		}

		WindowResult Win32Window::Startup(LPCSTR title, i32 width, i32 height, i32 nCmdShow, bool captureMouse)
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
			_hWnd = CreateWindow(NTEXT("ReENGINE"), title, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
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

			// Capture mouse if requested.
			if (captureMouse)
			{
				SetCapture(_hWnd);
				ShowCursor(false);
			}

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
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const auto action = (HIWORD(lParam) & KF_UP) ? Core::Input::Action::Release : Core::Input::Action::Press;
				if (action == Core::Input::Action::Release && wParam == VK_ESCAPE)
				{
					_shouldClose = true;
					PostQuitMessage(0);
					return 0;
				}
				else
				{
					if (codeToKey.contains(wParam))
					{
						// Invoke event handler passing the key code and action.
						KeyEvent(action, codeToKey[wParam]);
					}

					break;
				}
			}
			case WM_MOUSEMOVE:
			{
				tagPOINT centerCoordinates, mouseCoordinates;

				// Read mouse coordinates and convert to screen space.
				mouseCoordinates = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ClientToScreen(hWnd, &mouseCoordinates);

				if (_firstMouse)
				{
					// Store current mouse positions for posterior usage.
					_prevMouseX = mouseCoordinates.x;
					_prevMouseY = mouseCoordinates.y;

					_firstMouse = false;
					break;
				}

				// Invoke event handler passing the mouse position deltas.
				MouseEvent(mouseCoordinates.x - _prevMouseX, _prevMouseY - mouseCoordinates.y);

				// Reset cursor position to screen center.
				centerCoordinates = { _width / 2, _height / 2 };
				ClientToScreen(hWnd, &centerCoordinates);
				SetCursorPos(centerCoordinates.x, centerCoordinates.y);

				// Store current mouse positions for posterior usage.
				_prevMouseX = centerCoordinates.x;
				_prevMouseY = centerCoordinates.y;
				break;
			}

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

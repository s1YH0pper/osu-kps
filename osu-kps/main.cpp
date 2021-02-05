﻿// Copyright (c) UnnamedOrange. Licensed under the MIT Licence.
// See the LICENCE file in the repository root for full licence text.

#include <functional>

#include <Windows.h>
#undef min
#undef max

#include <utils/code_conv.hpp>
#include <utils/window.hpp>
#include <utils/timer_thread.hpp>

#include "integrated_kps.hpp"

/* void OnMoving(HWND hwnd, RECT* pRect) */
#define HANDLE_WM_MOVING(hwnd, wParam, lParam, fn)\
	((fn)((hwnd), (RECT*)(lParam)), 0L)
#define FORWARD_WM_MOVING(hwnd, pRect, fn)\
	(void)(fn)((hwnd), WM_MOVING, 0L, (LPARAM)(pRect))

class main_window : public window
{
	virtual INT_PTR WindowProc(HWND, UINT message, WPARAM wParam, LPARAM lParam) override
	{
		switch (message)
		{
			HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

			HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
			HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, OnNCLButtonDown);
			HANDLE_MSG(hwnd, WM_MOVING, OnMoving);

			HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

		case WM_DPICHANGED:
		{
			// UpdateDpiDependentFontsAndResources();

			RECT* const prcNewWindow = (RECT*)lParam;
			SetWindowPos(hwnd,
				NULL,
				prcNewWindow->left,
				prcNewWindow->top,
				prcNewWindow->right - prcNewWindow->left,
				prcNewWindow->bottom - prcNewWindow->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			break;
		}

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
		return 0;
	}
	BOOL OnCreate(HWND, LPCREATESTRUCT)
	{
		caption(L"osu!kps");
		SetWindowLongW(hwnd, GWL_STYLE, WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP | WS_SYSMENU); // 无边框窗口。
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		create_menu();

		return TRUE;
	}
	void OnDestroy(HWND)
	{
		DestroyMenu(hMenu);

		PostQuitMessage(0);
	}

	// 禁止窗口移出屏幕相关信息。
	mutable POINT pMouse{};
	mutable RECT rectWnd{};
	void OnLButtonDown(HWND, BOOL fDoubleClick, int x, int y, UINT)
	{
		if (fDoubleClick)
		{
			// TODO: 实现双击修改配置等功能。
		}
		else
		{
			PostMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));
		}
	}
	void OnNCLButtonDown(HWND, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
	{
		// 禁止窗口移出屏幕相关信息。
		GetWindowRect(hwnd, &rectWnd);
		GetCursorPos(&pMouse);
		FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, DefWindowProcW);
		PostMessageW(hwnd, WM_LBUTTONUP, 0, 0);
	}
	void OnMoving(HWND, RECT* pRect)
	{
		RECT& r = *pRect;
		POINT p;
		GetCursorPos(&p);
		RECT rWorkArea = work_area();
		int nWidth = r.right - r.left;
		int nHeight = r.bottom - r.top; // 获取窗口大小。
		rWorkArea.right -= nWidth; // 将窗口可能出现的位置更改为窗口左上角的点可能出现的位置。
		rWorkArea.bottom -= nHeight;

		r.left = rectWnd.left + (p.x - pMouse.x); // 根据鼠标位置初步得出窗口左上角位置。
		r.top = rectWnd.top + (p.y - pMouse.y);

		POINT pCorner = { r.left, r.top }; // 窗口左上角的点。
		if (pCorner.x < rWorkArea.left)
		{
			r.left = rWorkArea.left;
		}
		if (pCorner.y < rWorkArea.top)
		{
			r.top = rWorkArea.top;
		}
		if (pCorner.x > rWorkArea.right)
		{
			r.left = rWorkArea.right;
		}
		if (pCorner.y > rWorkArea.bottom)
		{
			r.top = rWorkArea.bottom;
		}
		r.right = r.left + nWidth; // 根据左上角位置和大小调整右下角。
		r.bottom = r.top + nHeight;
	}

	// 右键菜单。
	HMENU hMenu{};
	void create_menu()
	{
		// TODO: here
		hMenu = CreateMenu();

		HMENU hMenuPopup = CreateMenu();
		{
			HMENU hMenuKeys = CreateMenu();
			for (int i = 1; i <= 10; i++)
				AppendMenuW(hMenuKeys, MF_STRING, i, std::to_wstring(i).c_str());

			AppendMenuW(hMenuPopup, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuKeys), L"Button count");
		}
		{
			AppendMenuW(hMenuPopup, MF_STRING, id_exit, L"Exit");
		}

		AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuPopup), L"popup");
	}
	void OnRButtonDown(HWND, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{
		if (!fDoubleClick)
		{
			POINT p{ x, y };
			ClientToScreen(hwnd, &p);
			TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_RIGHTBUTTON, p.x, p.y,
				NULL, hwnd, nullptr);
		}
	}
	void OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify)
	{
		if (!hwndCtl) // 如果是菜单。
		{
			if (1 <= id && id <= 10) // 修改按键个数。
			{

			}
			else
				switch (id)
				{
				case id_exit:
				{
					PostMessageW(hwnd, WM_CLOSE, 0, 0);
					break;
				}
				default:
					break;
				}
		}
	}

public:
	inline static constexpr int id_exit = 11;

public:
	kps::kps kps;

public:
	double scale{ 1 }; // 绘图时的额外比例因子。
};

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	// 设置高 DPI 支持。
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	// 单例。
	HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"Global\\osu!kps-302F3137-DEA2-472F-9BC5-F078331E5576");
	DWORD last_error = GetLastError();
	if (last_error == ERROR_ALREADY_EXISTS ||
		last_error == ERROR_ACCESS_DENIED && !hMutex)
	{
		if (hMutex)
			CloseHandle(hMutex);
		return 0;
	}
	else if (!hMutex)
		return 1;

	// 窗口与主循环。
	{
		main_window wnd;
		wnd.create();
		wnd.message_loop();
	}

	// 关闭单例中用到的句柄。
	CloseHandle(hMutex);
	return 0;
}
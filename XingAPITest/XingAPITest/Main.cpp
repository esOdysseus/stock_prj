// XingAPITest.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include <Windows.h>

#define LOGTAG	"XingAPITest"
#include "dlogger.h"

//----------------------------------------------------------------------------------------------------
// 차트 지표데이터 이용 관련 정의
//----------------------------------------------------------------------------------------------------
#include "ChartIndex.h"					// 차트 지표데이터 조회용 TR의 헤더
#include "ChartExcel.h"					// 차트 엑셀데이터 조회용 TR의 헤더
#include "CStockMain.h"

#include "EScommon.h"

HWND gConsoleHWND = NULL;
HDC gConsoleHDC;
HINSTANCE gConsoleHInstance;

ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


CStockMain* pStock = NULL;

int main()
{
	ESLOG(LOG_DEBUG, "Enter.");

	MSG msg;
	gConsoleHInstance = GetModuleHandle(NULL);

	MyRegisterClass(gConsoleHInstance);

	//윈도우 객체 생성.
	gConsoleHWND = CreateWindowA("XingAPITest",
		"Test",
		WS_OVERLAPPEDWINDOW,
		10,// X
		100,// Y
		400,// Width
		400,// Height
		NULL,
		(HMENU)NULL,
		gConsoleHInstance,
		NULL);

	if (gConsoleHWND == NULL)
	{
		ESLOG(LOG_ERROR, "gConsoleHWND is NULL.(%X)", gConsoleHWND);
		return 0;
	}

	//윈도우 창 띄우기.
	ShowWindow(gConsoleHWND, 1);

	// Main Stock Class Create.
	if ((pStock = new CStockMain(gConsoleHWND)) == NULL)
	{
		ESLOG(LOG_ERROR, "CStockMain is NULL.");
		return 0;
	}

	if (pStock->isInit() == false)
	{
		ESLOG(LOG_ERROR, "Stock Initialization is failed.");
		return 0;
	}
	ESLOG(LOG_DEBUG, "Stock SDK initialization Success.");

	pStock->ST_Login();

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))	// GetMessage Function is blocking function. (If Message queue is empty, then this loop is blocking util incoming new message to message queue.)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (pStock)
	{
		// Stock Class Log-out/terminate/delete element.
		delete pStock;
		pStock = NULL;
	}

	ESLOG(LOG_DEBUG, "Exit.");
    return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T("XingAPITest");
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	return RegisterClassExW(&wcex);
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	case WM_NULL:
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			//case IDM_ABOUT:
			//	DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			//	break;
			//case IDM_EXIT:
			//	DestroyWindow(hWnd);
				//break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		//ESLOG(LOG_DEBUG, "message=%u", message);
		if (pStock != NULL && pStock->isInit() == true)
			pStock->ProcessMessage(message, wParam, lParam);

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}



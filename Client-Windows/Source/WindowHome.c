#include "window-home.h"
#include "updownctrl.h"

static HWND hGameCtlGroup;
static HWND hGameModeCBox;
static HWND hTimeCtlCBox;
static HWND hMinPerSideUDC;
static HWND hMinPerSideUDB;
static HWND hIncrementInSecUDC;
static HWND hIncrementInSecUDB;
static HWND hStartGameBtn;

INITCOMMONCONTROLSEX icex;

HDC		g_hMemDC;
HBITMAP g_hBackBuffer;
HBRUSH	g_backgroundBrush;
HFONT	g_font;
WNDPROC oldGameCtlProc;
WNDPROC oldMinPSideUDCProc;
WNDPROC oldIncPSecUDCProc;

static const UINT minValMinPerSideUDC = 0;
static const UINT maxValMinPerSideUDC = 180;
static const UINT minValIncrementInSecUDC = 0;
static const UINT maxValIncrementInSecUDC = 180;

static RECT GetHomeWindowRect() {
	RECT rect;
	HWND hWnd = FindWindow(g_szHomeWndClass, g_szHomeWndTitle);

	GetClientRect(hWnd, &rect);

	return rect;
}

static HDC GetBackBuffer(HDC hDC) {
	if (g_hMemDC) {
		return g_hMemDC;
	}
	RECT rect;
	rect = GetHomeWindowRect();

	g_hMemDC = CreateCompatibleDC(hDC);
	g_hBackBuffer = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);

	SelectObject(g_hMemDC, g_hBackBuffer);

	return g_hMemDC;
}

static void DeleteBackBuffer() {
	if (g_hMemDC) {
		DeleteDC(g_hMemDC);
		g_hMemDC = NULL;
	}

	if (g_hBackBuffer) {
		DeleteObject(g_hBackBuffer);
		g_hBackBuffer = NULL;
	}
}

static void InitializeBrushes() {
	if (!g_backgroundBrush) {
		g_backgroundBrush = CreateSolidBrush(RGB(18, 19, 17));
	}
}

static void InitializeFont() {
	g_font = CreateFont(
		18, 0, 0, 0, 
		FW_BOLD, (DWORD)NULL, (DWORD)NULL, (DWORD)NULL,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
		VARIABLE_PITCH, TEXT("Century Gothic")
	);
}
 
static void HandleWindowPaint(HWND hWnd, HDC hDC) {
	HDC hMemDC;
	RECT rect;
	GetClientRect(hWnd, &rect);

	hMemDC = GetBackBuffer(hDC);
	FillRect(hMemDC, &rect, g_backgroundBrush);

	BitBlt(hDC, 0, 0, rect.right, rect.bottom, g_hMemDC, 0, 0, SRCCOPY);
}

static LRESULT CALLBACK GameCtlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT paintStruct;
	HPEN hPen, hOldPen;
	HBRUSH hBrush, hOldBrush;

	switch (message) {
	case WM_PAINT: {
		hDC = BeginPaint(hWnd, &paintStruct);

		hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		hBrush = CreateSolidBrush(RGB(16, 16, 16));
		hOldPen = SelectObject(hDC, hPen);
		hOldBrush = SelectObject(hDC, hBrush);
		SelectObject(hDC, g_font);

		RECT rect;
		GetClientRect(hWnd, &rect);
		
		FillRect(hDC, &rect, hBrush);
		Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

		SelectObject(hDC, hOldBrush);
		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
		DeleteObject(hBrush);

		SetTextColor(hDC, RGB(255, 255, 255));
		SetBkMode(hDC, TRANSPARENT);

		TextOut(hDC, 90, 52,	L"Game Mode:",				11);
		TextOut(hDC, 95, 92,	L"Time control:",			14);
		TextOut(hDC, 66, 132,	L"Minutes per side:",		18);
		TextOut(hDC, 28, 172,	L"Increment in seconds:",	22);

		EndPaint(hWnd, &paintStruct);

		return 0;
	}
	default: 
		return CallWindowProc(oldGameCtlProc, hWnd, message, wParam, lParam);
	}
}

LRESULT CALLBACK HomeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT paintStruct;

	int winMsgId, winMsgEvent;
	
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);

	/*
	switch (message) {
	case WM_CREATE: {
		InitializeBrushes();
		InitializeFont();

		hGameCtlGroup = CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE,
			40, 120, 920, 600, hWnd, NULL,
			GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL
		);

		oldGameCtlProc = SetWindowLongPtr(hGameCtlGroup, GWLP_WNDPROC, (LONG_PTR)GameCtlProc);

		hGameModeCBox = CreateWindow(WC_COMBOBOX, TEXT(""),
			CBS_DROPDOWN | CBS_HASSTRINGS | CBS_SIMPLE | WS_CHILD  | WS_VISIBLE,
			200, 50, 200, 200, hGameCtlGroup, NULL, GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL
		);

		for (int i = 0; i < GameVariantCount; i++) {
			SendMessage(hGameModeCBox, CB_ADDSTRING, 0, (LPARAM)getGameVariantStr(i));
		}
		SendMessage(hGameModeCBox, CB_SELECTSTRING, 0, (LPARAM)getGameVariantStr(0));

		hTimeCtlCBox = CreateWindow(WC_COMBOBOX, TEXT(""),
			CBS_DROPDOWN | CBS_HASSTRINGS | CBS_SIMPLE | WS_CHILD | WS_VISIBLE,
			200, 90, 200, 200, hGameCtlGroup, NULL, GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL
		);

		for (int i = 0; i < TimeControlCount; i++) {
			SendMessage(hTimeCtlCBox, CB_ADDSTRING, 0, (LPARAM)getTimeControlStr(i));
		}
		SendMessage(hTimeCtlCBox, CB_SELECTSTRING, 0, (LPARAM)getTimeControlStr(0));
	
		hMinPerSideUDB = CreateUpDnBuddy(hGameCtlGroup, 200, 130, 200, 24);
		hMinPerSideUDC = CreateUpDnCtl(hGameCtlGroup, maxValMinPerSideUDC, minValMinPerSideUDC, 5);

		hIncrementInSecUDB = CreateUpDnBuddy(hGameCtlGroup, 200, 170, 200, 24);
		hIncrementInSecUDC = CreateUpDnCtl(hGameCtlGroup, maxValIncrementInSecUDC, minValIncrementInSecUDC, 3);

		hStartGameBtn = CreateWindow(WC_BUTTON, L"START GAME",
			BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			385, 540, 150, 50,
			hGameCtlGroup, NULL, GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL
		);
	}
	break;
	case WM_ERASEBKGND: {
		return 1;
	}
	break;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		RECT rect;
		rect = GetHomeWindowRect();
		
		SetWindowPos(hGameCtlGroup, HWND_TOP,
			(rect.right - 920) / 2, (rect.bottom - 640) / 2 + 60,
			0, 0, SWP_NOSIZE | SWP_ASYNCWINDOWPOS
		);

		DeleteBackBuffer();
	}
	case WM_PAINT: {
		hDC = BeginPaint(hWnd, &paintStruct);

		HandleWindowPaint(hWnd, hDC);

		EndPaint(hWnd, &paintStruct);
	}
	break;
	case WM_COMMAND: {
		winMsgId = LOWORD(wParam);
		winMsgEvent = HIWORD(wParam);

		switch (winMsgId) {
		case ID_FILE_EXIT: {
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = HOMEWND_WIDTH;
		lpMMI->ptMinTrackSize.y = HOMEWND_HEIGHT;
	}
	break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY: {
		PostQuitMessage(0);
	}
	break;
	default: {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	}*/
	return 0;

	ReleaseDC(hWnd, hDC);
}

void TerminateHomeWnd() {
	DeleteBackBuffer();
	DeleteObject(g_backgroundBrush);
	DeleteObject(g_font);
}

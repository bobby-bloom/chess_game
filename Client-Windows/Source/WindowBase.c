#include "window-base.h"

WCHAR g_szHomeWndTitle[MAX_LOAD_STRING];
WCHAR g_szHomeWndClass[MAX_LOAD_STRING];
WCHAR g_szGameWndTitle[MAX_LOAD_STRING];
WCHAR g_szGameWndClass[MAX_LOAD_STRING];

void RegisterHomeWndClass(HINSTANCE hInstance) {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = HomeWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAIN_MENU);
	wc.lpszClassName = g_szHomeWndClass;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClassExW(&wc)) {
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
	}
}

BOOL InitHomeWndInstance(HINSTANCE hInstance, int nCmdShow) {
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	RECT rect = { 0, 0, HOMEWND_WIDTH, HOMEWND_HEIGHT };

	AdjustWindowRectEx(&rect, dwStyle, FALSE, 0);

	g_hHomeWnd = CreateWindowW(
		g_szHomeWndClass,
		g_szHomeWndTitle,
		dwStyle,
		CW_USEDEFAULT, 0,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!g_hHomeWnd) {
		return FALSE;
	}

	ShowWindow(g_hHomeWnd, nCmdShow);
	UpdateWindow(g_hHomeWnd);

	return TRUE;
}

void RegisterGameWndClass(HINSTANCE hInstance) {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = GameWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAIN_MENU);
	wc.lpszClassName = g_szGameWndClass;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	
	if (!RegisterClassExW(&wc)) {
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
	}
}

BOOL InitGameWndInstance(HINSTANCE hInstance, int nCmdShow) {
	DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;

	RECT rect = { 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT + 20 };

	AdjustWindowRectEx(&rect, dwStyle, FALSE, 0);

	g_hGameWnd = CreateWindowW(
		g_szGameWndClass,
		g_szGameWndTitle,
		dwStyle,
		CW_USEDEFAULT, 0,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!g_hGameWnd) {
		return FALSE;
	}

	ShowWindow(g_hGameWnd, nCmdShow);
	UpdateWindow(g_hGameWnd);
	
	return TRUE;
}
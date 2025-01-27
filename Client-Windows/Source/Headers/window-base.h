#ifndef WINDOW_BASE_H
#define WINDOW_BASE_H

#include <Windows.h>
#include "resource.h"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define MAX_LOAD_STRING 100

#define HOMEWND_WIDTH 1000
#define HOMEWND_HEIGHT 800

#define GAMEWND_WIDTH 800
#define GAMEWND_HEIGHT GAMEWND_WIDTH

HWND g_hHomeWnd, g_hGameWnd;

extern HINSTANCE g_hInstance;
extern WCHAR g_szHomeWndTitle[MAX_LOAD_STRING];
extern WCHAR g_szHomeWndClass[MAX_LOAD_STRING];
extern WCHAR g_szGameWndTitle[MAX_LOAD_STRING];
extern WCHAR g_szGameWndClass[MAX_LOAD_STRING];

extern LRESULT CALLBACK HomeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK GameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void RegisterHomeWndClass(HINSTANCE hInstance);
void RegisterGameWndClass(HINSTANCE hInstance);
BOOL InitHomeWndInstance(HINSTANCE hInstance, int nCmdShow);
BOOL InitGameWndInstance(HINSTANCE hInstance, int nCmdShow);

#endif // WINDOW_BASE_H
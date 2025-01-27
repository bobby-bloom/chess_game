#include <entry-point.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
	HACCEL hHomeAccelTable;
	HACCEL hGameAccelTable;

	g_hInstance = hInstance;

#ifdef DEBUG
	InitDebugConsole();
#endif

	LoadStringW(hInstance, IDS_APP_TITLE_HOME,	g_szHomeWndTitle,	MAX_LOAD_STRING);
	LoadStringW(hInstance, IDS_APP_TITLE_GAME,	g_szGameWndTitle,	MAX_LOAD_STRING);
	LoadStringW(hInstance, IDC_CHESS_HOME,		g_szHomeWndClass,	MAX_LOAD_STRING);
	LoadStringW(hInstance, IDC_CHESS_GAME,		g_szGameWndClass,	MAX_LOAD_STRING);

	RegisterHomeWndClass(hInstance);
	RegisterGameWndClass(hInstance);

	runDefaultGame();

	if (!InitGameWndInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	hHomeAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHESS_HOME));
	hGameAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHESS_GAME));

	while (GetMessage(&msg, NULL, 0, 0) > 0)	 {
		if (!TranslateAccelerator(msg.hwnd, hHomeAccelTable, &msg)
			&& !TranslateAccelerator(msg.hwnd, hGameAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#ifdef DEBUG
	TerminateConsole();
#endif
	TerminateHomeWnd();
	TerminateGameWnd();
	return msg.wParam;
}
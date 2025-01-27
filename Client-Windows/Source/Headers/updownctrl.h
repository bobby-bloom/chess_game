#ifndef UPDOWNCTRL_H
#define UPDOWNCTRL_H

#include "Windows.h"
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")

HWND CreateUpDnBuddy(HWND hwndParent, int posX, int posY, int width, int height);
HWND CreateUpDnCtl(HWND hwndParent, const UINT minVal, const UINT maxVal, const UINT initVal);
extern HINSTANCE g_hInstance;
extern INITCOMMONCONTROLSEX icex;
#endif // UPDOWNCTRL_H

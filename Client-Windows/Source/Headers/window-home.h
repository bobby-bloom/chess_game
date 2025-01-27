#ifndef WINDOW_HOME_H
#define WINDOW_HOME_H

#include "window-base.h"
#include "game.h"

LRESULT CALLBACK HomeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void TerminateHomeWnd();

#endif // WINDOW_HOME_H
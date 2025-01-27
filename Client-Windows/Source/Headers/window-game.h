#ifndef WINDOW_CHESS_H
#define WINDOW_CHESS_H

#pragma comment( lib, "Msimg32" ) 

#include <cairo.h>
#include <cairo-win32.h>
#include <librsvg-2.0/librsvg/rsvg.h>

#include "core.h"
#include "window-base.h"

static const int g_fieldSize = GAMEWND_WIDTH / BOARD_COLS;

LRESULT CALLBACK GameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void			 TerminateGameWnd();

#endif // WINDOW_CHESS_H
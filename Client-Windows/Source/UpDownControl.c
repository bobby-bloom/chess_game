#include "updownctrl.h"

RECT rcClient;

HWND hControl = NULL;
HWND hwndUpDnEdtBdy = NULL;
HWND hwndUpDnCtl = NULL;

HWND CreateUpDnBuddy(HWND hwndParent, int posX, int posY, int width, int height) {
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE | WS_EX_CONTEXTHELP,
        WC_EDIT,
        NULL,
        WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER 
        | ES_NUMBER | ES_LEFT,    
        posX, posY,
        width, height,
        hwndParent,
        NULL,
        g_hInstance,
        NULL);

    return (hControl);
}

HWND CreateUpDnCtl(HWND hwndParent, const UINT minVal, const UINT maxVal, const UINT initVal) {
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);  

    hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING,
        UPDOWN_CLASS,
        NULL,
        WS_CHILDWINDOW | WS_VISIBLE
        | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
        0, 0,
        0, 0,         // Set to zero to automatically size to fit the buddy window.
        hwndParent,
        NULL,
        g_hInstance,
        NULL);

    SendMessage(hControl, UDM_SETRANGE, 0, MAKELPARAM(minVal, maxVal));
    SendMessage(hControl, UDM_SETPOS, 0, MAKELPARAM(initVal, 0));

    return (hControl);
}
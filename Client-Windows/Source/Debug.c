#include "debug.h"

void InitDebugConsole() {
	 AllocConsole();

	 FILE* fp;
	 freopen_s(&fp, "CONOUT$", "w", stdout);

	 freopen_s(&fp, "CONOUT$", "w", stderr);

	 printf("Console initialized\n");
 }

void TerminateConsole() {
	 FreeConsole();
 }
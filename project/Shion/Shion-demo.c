#include <stdio.h>
#include <Windows.h>

#include "../../src/Shion.h"

static PSHION_HOOK g_MessageBoxAHook = NULL;

int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	typedef int(WINAPI* FuncMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

	printf("MessageBoxA %d %s %s %d\r\n", (DWORD)hWnd, lpText, lpCaption, uType);
#ifndef _DEBUG
	if (g_MessageBoxAHook != NULL && g_MessageBoxAHook->OriginalSize)
		return ((FuncMessageBoxA)&g_MessageBoxAHook->OriginalData)(hWnd, "Shion daisuki", lpCaption, uType);
#endif // _DEBUG
	return 0;
}

int main() {

	g_MessageBoxAHook = malloc(sizeof(SHION_HOOK));
	memset(g_MessageBoxAHook, 0, sizeof(SHION_HOOK));

	do {
		MessageBoxA(NULL, "Hello Shion", "Tip", 0);

		if (!ShionHook(&MessageBoxA, &MyMessageBoxA, g_MessageBoxAHook)) {
			printf("MessageBoxA hook fail\n");
			break;
		}

		MessageBoxA(NULL, "Hello Shion", "Tip", 0);

		if (!ShionRestore(g_MessageBoxAHook)) {
			printf("MessageBoxA unhook fail\n");
			break;
		}
		MessageBoxA(NULL, "Shion Shion Shion", "Tip", 0);

		return 0;
	} while (0);

	return -1;
}
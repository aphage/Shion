#pragma once
#ifndef _SHION_H
#define _SHION_H

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
	typedef struct _SHION_HOOK {
		PVOID OriginalPtr;      // Original function VA
		ULONG OriginalSize;     // Size of original data
		UCHAR OriginalData[80]; // Original bytes + jump
	}SHION_HOOK,*PSHION_HOOK;
#pragma pack(pop)

	BOOL ShionHook(_In_ PVOID pFunc, _In_ PVOID pNewFunc, _Inout_ PSHION_HOOK pHook);

	BOOL ShionRestore(_In_ PSHION_HOOK pHook);

#ifdef __cplusplus
}
#endif
#endif//_SHION_H


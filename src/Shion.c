/*
The MIT License (MIT)

Copyright (c) 2016 DarthTon
Copyright (c) 2020 aphage

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "LDasm.h"
#include "Shion.h"
#include <Windows.h>

#pragma comment(lib,"ntdll.lib")

#pragma pack(push, 1)
typedef struct _JUMP_THUNK {
	UCHAR PushOp;           // 0x68
	ULONG AddressLow;       // 
#ifdef _WIN64
	ULONG MovOp;            // 0x042444C7
	ULONG AddressHigh;      // 
#endif
	UCHAR RetOp;            // 0xC3
} JUMP_THUNK, * PJUMP_THUNK;
#pragma pack(pop)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
	_In_ HANDLE ProcessHandle,
	_Inout_ PVOID* BaseAddress,
	_Inout_ PSIZE_T RegionSize,
	_In_ ULONG NewProtect,
	_Out_ PULONG OldProtect
	);

void*
aqua_memmove(void* dest, const void* src, size_t len);

__forceinline
__int64
aqua_abs64(__int64 i);

BOOL ShionVirtualProtect(_In_  LPVOID lpAddress, _In_  SIZE_T dwSize, _In_  DWORD flNewProtect, _Out_ PDWORD lpflOldProtect) {

	return !NtProtectVirtualMemory((HANDLE)-1, &lpAddress, &dwSize, flNewProtect, lpflOldProtect);
}

void ShionInitJumpThunk(_Inout_ PJUMP_THUNK pThunk, _In_ ULONG64 To) {

	PULARGE_INTEGER liTo = (PULARGE_INTEGER)&To;

	pThunk->PushOp = 0x68;
	pThunk->AddressLow = liTo->LowPart;
#ifdef _WIN64
	pThunk->MovOp = 0x042444C7;
	pThunk->AddressHigh = liTo->HighPart;
#endif
	pThunk->RetOp = 0xC3;
}

BOOL ShionCopyCode(_In_ PVOID pFunc, _Out_ PUCHAR OriginalStore, _Out_ PULONG pSize) {
	// Store original bytes
	PUCHAR src = (PUCHAR)pFunc;
	PUCHAR old = OriginalStore;
	ULONG all_len = 0;
	ldasm_data ld = { 0 };

	do
	{
#ifdef _WIN64
		ULONG len = ldasm(src, &ld, TRUE);
#else
		ULONG len = ldasm(src, &ld, FALSE);
#endif

		// Determine code end
		if (ld.flags & F_INVALID
			|| (len == 1 && (src[ld.opcd_offset] == 0xCC || src[ld.opcd_offset] == 0xC3))
			|| (len == 3 && src[ld.opcd_offset] == 0xC2)
			|| len + all_len > 128)
		{
			break;
		}

		// move instruction 
		aqua_memmove(old, src, len);

		// if instruction has relative offset, calculate new offset 
		if (ld.flags & F_RELATIVE)
		{
			LONG diff = 0;
			const uintptr_t ofst = (ld.disp_offset != 0 ? ld.disp_offset : ld.imm_offset);
			const uintptr_t sz = ld.disp_size != 0 ? ld.disp_size : ld.imm_size;

			aqua_memmove(&diff, src + ofst, sz);
			// exit if jump is greater then 2GB
			if (aqua_abs64(src + len + diff - old) > INT_MAX)
			{
				break;
			}
			else
			{
				diff += (LONG)(src - old);
				aqua_memmove(old + ofst, &diff, sz);
			}
		}

		src += len;
		old += len;
		all_len += len;

	} while (all_len < sizeof(JUMP_THUNK));

	// Failed to copy old code, use backup plan
	if (all_len < sizeof(JUMP_THUNK))
	{
		return FALSE;
	}
	else
	{
		ShionInitJumpThunk((PJUMP_THUNK)old, (ULONG64)src);
		*pSize = all_len;
	}

	return TRUE;
}

BOOL ShionHook(_In_ PVOID pFunc, _In_ PVOID pNewFunc, _Inout_ PSHION_HOOK pHook) {
	if (pFunc == NULL || pHook == NULL)
		return FALSE;

	if (ShionCopyCode(pFunc, pHook->OriginalData, &pHook->OriginalSize)) {
		JUMP_THUNK thunk;

		ShionInitJumpThunk(&thunk, (ULONG64)pNewFunc);
		ULONG oldProtect = 0;
		if (ShionVirtualProtect(pFunc, sizeof(thunk), PAGE_EXECUTE_READWRITE, &oldProtect)) {
			aqua_memmove(pFunc, &thunk, sizeof(thunk));
			ShionVirtualProtect(pFunc, sizeof(thunk), oldProtect, &oldProtect);

			ShionVirtualProtect(pHook->OriginalData, pHook->OriginalSize, PAGE_EXECUTE_READWRITE, &oldProtect);
			pHook->OriginalPtr = pFunc;
		}
		return TRUE;
	}

	return FALSE;
}

BOOL ShionRestore(_In_ PSHION_HOOK pHook) {
	if (pHook == NULL)
		return FALSE;
	ULONG oldProtect = 0;
	if (ShionVirtualProtect(pHook->OriginalPtr, pHook->OriginalSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		aqua_memmove(pHook->OriginalPtr, pHook->OriginalData, pHook->OriginalSize);
		ShionVirtualProtect(pHook->OriginalPtr, pHook->OriginalSize, oldProtect, &oldProtect);
		return TRUE;
	}
	return FALSE;
}

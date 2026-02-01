#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifndef WINAPI
#  define WINAPI __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif
	DWORD WINAPI MyGetFileVersionInfoSizeW(LPCWSTR, LPDWORD);
	BOOL  WINAPI MyGetFileVersionInfoW(LPCWSTR, DWORD, DWORD, LPVOID);
	BOOL  WINAPI MyVerQueryValueW(LPCVOID, LPCWSTR, LPVOID*, PUINT);
	DWORD WINAPI MyGetModuleFileNameW(LPVOID hModule, LPWSTR lpFilename, DWORD nSize);
#ifdef __cplusplus
}
#endif


#define GetFileVersionInfoSizeW   MyGetFileVersionInfoSizeW
#define GetFileVersionInfoW       MyGetFileVersionInfoW
#define VerQueryValueW            MyVerQueryValueW
#define GetModuleFileNameW            MyGetModuleFileNameW
#define DllMain optisc_original_DllMain

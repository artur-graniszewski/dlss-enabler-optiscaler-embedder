// main_wrapper.cpp

#include <windows.h>  

extern "C" {
	BOOL WINAPI optisc_original_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
}

//#define DllMain optisc_original_DllMain

#include "dllmain.cpp"

//#undef DllMain
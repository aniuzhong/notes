#pragma once

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <stdio.h>
#define DLL_EXPORT_NACPUTERAUDIO extern "C" __declspec(dllexport) 
#else
#define DLL_EXPORT_NACPUTERAUDIO extern "C"
#endif //_WINDOWS
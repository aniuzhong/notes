#pragma once

#include <string>
#ifdef _WINDOWS
#else
#include <stdint.h>
#endif

bool GetCurrentLibraryDir(std::string & str_dir);
void* XxxxLoadLibrary(const char * path);
void XxxxFreeLibrary(void * library_handle);
void* XxxxGetProcAddress(void * library_handle, const char * function_name);
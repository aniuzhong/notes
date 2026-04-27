#ifdef _WINDOWS
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
extern HMODULE g_hModule;
#else
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#endif // _WINDOWS

#include <cassert>
#include <iostream>

#include <CommonFile/XxxxPlayerLog.h>

#include "Common.h"

#ifndef _WINDOWS
static int32_t GetCurrentPathTest()
{
    return 0;
}
#endif

bool GetCurrentLibraryDir(std::string & str_dir)
{
#ifdef _WINDOWS
    char path[260] = { 0 };
    GetModuleFileNameA(g_hModule, path, 260);
    PathRemoveFileSpecA(path);
    str_dir = path;
    return true;
#else
    void* addr = reinterpret_cast<void*>(&GetCurrentPathTest);

    Dl_info info;
    if (dladdr(addr, &info))
    {
        std::string str_parent_path = info.dli_fname;
        std::filesystem::path parent_path =  str_parent_path;
        str_dir = parent_path.parent_path().string();
        return true;
    } 
    else 
    {
        std::cerr << "Failed to get library path." << std::endl;
        assert(0);
        return false;
    }
#endif 
}

void * XxxxLoadLibrary(const char * path)
{
#ifdef _WINDOWS
        HMODULE hook_dll = LoadLibraryA(path);
#else
        void* hook_dll = dlopen(path, RTLD_LAZY);
#endif
        if(!hook_dll)
        {
#ifdef _WINDOWS
            NPLOG_ERROR("LoadLibrary faild, path is : %s, err=%d.\n", path, GetLastError());
#else
            NPLOG_ERROR("LoadLibrary faild, path is : %s, error is: %s", path, dlerror());
#endif
        }
    return hook_dll;
}

void XxxxFreeLibrary(void * library_handle)
{
    if(library_handle)
    {
#ifdef _WINDOWS
        FreeLibrary((HMODULE)library_handle);
#else
        dlclose(library_handle);
#endif
    }
}

void* XxxxGetProcAddress(void * library_handle, const char * function_name)
{
    if(library_handle && function_name)
    {
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library_handle, function_name);
#else
    return dlsym(library_handle, function_name);
#endif
    }
    return nullptr;
}

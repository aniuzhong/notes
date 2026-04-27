#include <string>

#include <CommonFile/XxxxPlayerLog.h>

#include "Common.h"
#include "XxxxAudioCapturePlayerExport.h"

NP_AUDIO_DEVICE_INFO_NODE* NPEnumAudioCaptureDevices(NP_ENUM_AUDIO_CAPTURE_DEVICE_TYPE enum_type)
{
    static void* audiocapture_dll = nullptr;
    std::string library_path;
    GetCurrentLibraryDir(library_path);
#ifdef _WINDOWS
    library_path.append("\\XxxxAudioCaptureCore.dll");
#else
    if (!library_path.empty())
    {
        library_path.append("/");
    }
    library_path.append("libXxxxAudioCaptureCore.so");
#endif
    audiocapture_dll = XxxxLoadLibrary(library_path.c_str());
    if (!audiocapture_dll)
    {
        NPLOG_ERROR("XxxxLoadLibrary %s failed.\n", library_path.c_str());
        XxxxFreeLibrary(audiocapture_dll);
        return nullptr;
    }

    auto NACSetLogCallback = reinterpret_cast<pfnNSomeSetLogCallback>(XxxxGetProcAddress(audiocapture_dll, "NACSetLogCallback"));

    if (NACSetLogCallback)
    {
        NACSetLogCallback(_LOG_Message);
        NPLOG_INFO("Message from XxxxAudioCaptureCore");
    }

    XxxxFreeLibrary(audiocapture_dll);
    return nullptr;
}
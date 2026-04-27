#ifndef __XxxxPLAYER_LOG_H__
#define __XxxxPLAYER_LOG_H__

#include <cstdint>
#include <cstring>

// XxxxPlayerLogDefine.h
#ifndef _H_XxxxPLAYER_LOG_LEVEL_H_
#define _H_XxxxPLAYER_LOG_LEVEL_H_
enum XxxxPLAYER_LOG_LEVEL
{
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_CRITICAL,
};
#endif

typedef void (*XxxxLogCallback)(XxxxPLAYER_LOG_LEVEL, const char* log_message);
typedef void (*pfnNSomeSetLogCallback)(XxxxLogCallback callback);

#ifdef _WIN32
#define __func__ __FUNCTION__
#define __BASE_FILE__ \
(strrchr(__FILE__, '/') \
    ?strrchr(__FILE__, '/') + 1 \
    :(strrchr(__FILE__, '\\') \
        ? strrchr(__FILE__, '\\') + 1 \
        : __FILE__))
#endif

// Steps for Adding Log Export Support to a Submodule in NPE (Using the Example Submodule)
//
// 1. Add the following declaration to /XxxxPlayer/CommonFile/XxxxPlayerLog.h
// 
//        void NExampleSetLogCallbackImpl(XxxxLogCallback callback);
// 
//    Then, implement it in /XxxxPlayer/CommonFile/XxxxPlayerLog.cpp:
// 
//        void NExampleSetLogCallbackImpl(XxxxLogCallback callback) {
//            printf("NExampleSetLogCallbackImpl: %p\n", &g_log_callback);
//            g_log_callback = callback;
//        }
//
// 2. Add an exported function to the corresponding submodule:
//
//        void NExampleSetLogCallback(XxxxLogCallback callback) {
//            NExampleSetLogCallbackImpl(callback);
//        }
//
// 3. Invoke the exported function upon submodule import
// 
//        auto cb = reinterpret_cast<pfnNSomeSetLogCallback>(XxxxGetProcAddress(hdll, "NExampleSetLogCallback"));
//        cb(_LOG_Message);
//

// [注] 在 Linux 系统下，
// 1. 需在各子模块的 CMakeLists.txt 中添加 XxxxPlayerLog.cpp
// 2. 需在各子模块的 CMakeLists.txt 中添加编译选项 -fPIC 以确保指针的传递
// 3. 需在各子模块的 CMakeLists.txt 中添加编译选项 -Bsymbolic 来确保符号的唯一性，避免动态加载冲突
//    见 [https://maskray.me/blog/2021-05-16-elf-interposition-and-bsymbolic]

void NPSetLogCallbackImpl(XxxxLogCallback callback); // XxxxPlayer
void NSSetLogCallbackImpl(XxxxLogCallback callback); // XxxxSender
void NSCSetLogCallbackImpl(XxxxLogCallback callback); // XxxxSequenceCore
void NACSetLogCallbackImpl(XxxxLogCallback callback); // XxxxAudioCaptureCore
void NECSetLogCallbackImpl(XxxxLogCallback callback); // XxxxEncoderCore
void NNDISenderSetLogCallbackImpl(XxxxLogCallback callback); // XxxxNDISenderCore
void NBMDSetLogCallbackImpl(XxxxLogCallback callback); // XxxxBMDCore
void NSSpoutSetLogCallbackImpl(XxxxLogCallback callback); // XxxxSpout
void NIPCSetLogCallbackImpl(XxxxLogCallback callback); // XxxxImageProcessCore
void NTCSetLogCallbackImpl(XxxxLogCallback callback); // XxxxTranscodeCore
void NVCSetLogCallbackImpl(XxxxLogCallback callback); // XxxxVideoCaptureCore
void NTCConvertSetLogCallbackImpl(XxxxLogCallback callback); // XxxxTextureConversion

void _LOG(XxxxPLAYER_LOG_LEVEL logLevel, const char* file, const char* func, unsigned int line, const char* format, ...);
void _LOG_Message(XxxxPLAYER_LOG_LEVEL logLevel, const char* message);

#define NPLOG(logLevel, format, ...) _LOG(logLevel, __BASE_FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define NPLOG_ERROR(format, ...) NPLOG(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define NPLOG_WARNING(format, ...) NPLOG(LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define NPLOG_INFO(format, ...) NPLOG(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define NPLOG_DEBUG(format, ...) NPLOG(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#endif
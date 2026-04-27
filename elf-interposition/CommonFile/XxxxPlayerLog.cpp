#ifdef _WINDOWS
#include <windows.h>
#endif // _WINDOWS

#ifdef __linux__
#include <sys/time.h>
#endif // __linux__

#include <cstdarg>
#include <iostream>

#include "XxxxPlayerLog.h"

static const char* XxxxPLAYER_LOG_LEVEL_PREFIX[] =
{
    "ERROR:",
    "WARNNING:",
    "INFO:",
    "DEBUG:",
    "CRITICAL:"
};

static XxxxLogCallback g_log_callback = nullptr;

void NPSetLogCallbackImpl(XxxxLogCallback callback) { printf("NPSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NSSetLogCallbackImpl(XxxxLogCallback callback) { printf("NSSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NSCSetLogCallbackImpl(XxxxLogCallback callback) { printf("NSCSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NACSetLogCallbackImpl(XxxxLogCallback callback) { printf("NACSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NECSetLogCallbackImpl(XxxxLogCallback callback) { printf("NECSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NNDISenderSetLogCallbackImpl(XxxxLogCallback callback) { printf("NNDISenderSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NBMDSetLogCallbackImpl(XxxxLogCallback callback) { printf("NBMDSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NSSpoutSetLogCallbackImpl(XxxxLogCallback callback) { printf("NSSpoutSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NIPCSetLogCallbackImpl(XxxxLogCallback callback) { printf("NIPCSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NTCSetLogCallbackImpl(XxxxLogCallback callback) { printf("NTCSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NVCSetLogCallbackImpl(XxxxLogCallback callback) { printf("NVCSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }
void NTCConvertSetLogCallbackImpl(XxxxLogCallback callback) { printf("NTCConvertSetLogCallbackImpl: %p\n", &g_log_callback); g_log_callback = callback; }

void _LOG_Message(XxxxPLAYER_LOG_LEVEL logLevel, const char* message)
{
    if (g_log_callback)
    {
        g_log_callback(logLevel, message);
    }
    else
    {
        char date[100] = { 0 };
#ifdef _WINDOWS
        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);

        sprintf_s(date, 100, "[%d-%02d-%02d %02d:%02d:%02d.%03d] ",
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds
        );
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm* ptm = localtime(&(tv.tv_sec));
        long milliseconds = tv.tv_usec / 1000;
        char time_string[40];
        strftime(time_string, sizeof(time_string), "[%Y-%02m-%02d %02H:%02M:%02S", ptm);
        snprintf(date, sizeof(date), "%s.%03ld] ", time_string, milliseconds);
#endif
        std::cout << date << XxxxPLAYER_LOG_LEVEL_PREFIX[logLevel] << message << std::endl;
    }
}

void _LOG(XxxxPLAYER_LOG_LEVEL logLevel, const char* file, const char* func, uint32_t line, const char* format, ...)
{
    if (logLevel > LOG_LEVEL_CRITICAL)
        return;

    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, 512, format, args);
    va_end(args);

    if (g_log_callback)
    {
        g_log_callback(logLevel, message);
    }
    else
    {
        char date[100] = { 0 };
#ifdef _WINDOWS
        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);

        sprintf_s(date, 100, "[%d-%02d-%02d %02d:%02d:%02d.%03d] ",
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds
        );
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm* ptm = localtime(&(tv.tv_sec));
        long milliseconds = tv.tv_usec / 1000;
        char time_string[40];
        strftime(time_string, sizeof(time_string), "[%Y-%02m-%02d %02H:%02M:%02S", ptm);
        snprintf(date, sizeof(date), "%s.%03ld] ", time_string, milliseconds);
#endif
        if (LOG_LEVEL_ERROR == logLevel)
        {
            std::cerr << date << XxxxPLAYER_LOG_LEVEL_PREFIX[logLevel] << file << ":" << line << " " << func << " ";
            std::cerr << message << std::endl;
        }
        else
        {
            std::cout << date << XxxxPLAYER_LOG_LEVEL_PREFIX[logLevel] << message << std::endl;
        }
    }
}
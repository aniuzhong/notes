#ifndef DEBUG_H
#define DEBUG_H

#ifdef _WIN32
#define __func__ __FUNCTION__
#define __BASE_FILE__ \
(strrchr(__FILE__, '/') \
    ?strrchr(__FILE__, '/') + 1 \
    :(strrchr(__FILE__, '\\') \
        ? strrchr(__FILE__, '\\') + 1 \
        : __FILE__))
#endif

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif
#include "XxxxPlayer/XxxxPlayerDefine.h"

class CLogCallback
{
public:
    static XxxxLogCallback g_log_callback;
};

using namespace std;

const static char* _LOG_LEVEL_PREFIX[] =
{
    "ERROR:",
    "WARNNING:",
    "INFO:",
    "DEBUG:",
    "CRITICAL:"
};

static void _LOG_Message(XxxxPLAYER_LOG_LEVEL logLevel, const char* message)
{

    if (CLogCallback::g_log_callback)
    {
        CLogCallback::g_log_callback(logLevel, message);
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
        struct tm* ptm = localtime (&(tv.tv_sec));
        long milliseconds = tv.tv_usec / 1000;
        char time_string[40];
        strftime (time_string, sizeof(time_string), "[%Y-%02m-%02d %02H:%02M:%02S", ptm);
        snprintf (date, sizeof(date), "%s.%03ld] ", time_string, milliseconds);
#endif
        std::cout << date << _LOG_LEVEL_PREFIX[logLevel] << message << std::endl;
    }
}

static void _LOG(XxxxPLAYER_LOG_LEVEL logLevel, const char* file, const char* func, uint32_t line, const char* format, ...)
{
    if (logLevel > LOG_LEVEL_CRITICAL)
        return;

    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, 512, format, args);
    va_end(args);

    if (CLogCallback::g_log_callback)
    {
        CLogCallback::g_log_callback(logLevel, message);
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
        struct tm* ptm = localtime (&(tv.tv_sec));
        long milliseconds = tv.tv_usec / 1000;
        char time_string[40];
        strftime (time_string, sizeof(time_string), "[%Y-%02m-%02d %02H:%02M:%02S", ptm);
        snprintf (date, sizeof(date), "%s.%03ld] ", time_string, milliseconds);
#endif
        if (LOG_LEVEL_ERROR == logLevel)
        {
            std::cerr << date << _LOG_LEVEL_PREFIX[logLevel] << file << ":" << line << " " << func << " ";
            std::cerr << message << std::endl;
        }
        else
        {
            std::cout << date << _LOG_LEVEL_PREFIX[logLevel] << message << std::endl;
        }
    }
}

static void _SetLogCallback(XxxxLogCallback callback)
{
    CLogCallback::g_log_callback = callback;
}

#define NPLOG(logLevel, format, ...) \
    _LOG(logLevel, __BASE_FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define NPLOG_ERROR(format, ...) \
    NPLOG(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#define NPLOG_WARNING(format, ...) \
    NPLOG(LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

#define NPLOG_INFO(format, ...) \
    NPLOG(LOG_LEVEL_INFO, format, ##__VA_ARGS__)

#define NPLOG_DEBUG(format, ...) \
    NPLOG(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)


class GlobalLogSet
{
private:
    GlobalLogSet();
    virtual ~GlobalLogSet();
    GlobalLogSet(const GlobalLogSet&) = delete;
    GlobalLogSet& operator= (GlobalLogSet&) = delete;
public:
    static GlobalLogSet* GetInstance();
    void OpenDebugLog(bool openDebuLog);
    bool IsDebugLogOpen();
private:
    bool m_openDebuLog = false;
};

#endif
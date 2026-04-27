#pragma once

#include "framework.h"

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

DLL_EXPORT_NACPUTERAUDIO void NACSetLogCallback(XxxxLogCallback callback);
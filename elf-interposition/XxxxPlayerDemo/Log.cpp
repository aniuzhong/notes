#include "Log.h"

XxxxLogCallback CLogCallback::g_log_callback = nullptr;


GlobalLogSet::GlobalLogSet()
{
    m_openDebuLog = false;
}

GlobalLogSet::~GlobalLogSet()
{

}

GlobalLogSet* GlobalLogSet::GetInstance()
{
    static GlobalLogSet obj;
    return &obj;
}

void GlobalLogSet::OpenDebugLog(bool openDebuLog)
{
    m_openDebuLog = openDebuLog;
}

bool GlobalLogSet::IsDebugLogOpen()
{
    return m_openDebuLog;
}
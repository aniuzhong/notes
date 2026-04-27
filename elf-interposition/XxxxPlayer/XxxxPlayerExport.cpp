#include <CommonFile/XxxxPlayerLog.h>

#include "XxxxPlayer.h"

void NPSetLogCallback(XxxxLogCallback callback)
{
    NPSetLogCallbackImpl(callback);
    NPLOG_DEBUG("Xxxxplayer module log callback (0x%zx) setup successful",
                reinterpret_cast<std::uintptr_t>(callback));
}

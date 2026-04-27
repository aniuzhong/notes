#include <CommonFile/XxxxPlayerLog.h>

#include "AudioCaptureExport.h"

void NACSetLogCallback(XxxxLogCallback callback)
{
    NACSetLogCallbackImpl(callback);
    NPLOG_DEBUG("XxxxAudioCaptureCore module log callback (0x%zx) setup successful",
                reinterpret_cast<std::uintptr_t>(callback));
}
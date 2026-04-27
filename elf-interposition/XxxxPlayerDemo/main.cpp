#include <filesystem>
#include <fstream>

#include <XxxxPlayer/IXxxxPlayer.h>
#include <XxxxPlayer/XxxxAudioCapturePlayerExport.h>

#include "Log.h"

static std::string LogLevelToString(XxxxPLAYER_LOG_LEVEL level)
{
    switch (level)
    {
        case LOG_LEVEL_DEBUG:    return "DEBUG";
        case LOG_LEVEL_INFO:     return "INFO";
        case LOG_LEVEL_WARNING:  return "WARN";
        case LOG_LEVEL_ERROR:    return "ERROR";
        case LOG_LEVEL_CRITICAL: return "FATAL";
        default:                 return "UNKNOWN";
    }
}

// Windows    "C:\\Users\\<User Name>\\AppData\\Local\\Temp\\Xxxxplayerlog.txt"
// Linux      "/tmp/Xxxxplayerlog.txt"
static void XxxxLogToFile(XxxxPLAYER_LOG_LEVEL level, const char* log_message)
{
    std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
    std::filesystem::path log_file = tmp_path / "Xxxxplayerlog.txt";
    std::ofstream ofs(log_file, std::ios::app);
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    ofs << "[" << std::put_time(std::localtime(&now_c), "%F %T") << "] "
        << "[" << LogLevelToString(level) << "] " << log_message << std::endl;
}

int main(int argc, char* argv[])
{
    // NPSetLogCallback(XxxxLogToFile);

    NPLOG_INFO("XxxxPlayerDemo!main");
    NPEnumAudioCaptureDevices(NP_ENUM_AUDIO_CAPTURE_NONE_DEVICE);

    return 0;
}
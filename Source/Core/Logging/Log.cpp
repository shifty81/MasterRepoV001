#include "Core/Logging/Log.h"
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <unordered_map>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace NF {

namespace {

// ---------------------------------------------------------------------------
// Global state — protected by s_Mutex
// ---------------------------------------------------------------------------
LogLevel                                         s_MinLevel = LogLevel::Trace;
std::mutex                                       s_Mutex;
bool                                             s_FileLoggingActive = false;
std::string                                      s_LogRoot;
std::ofstream                                    s_MasterFile;   // NovaForge.log
std::unordered_map<std::string, std::ofstream>   s_ChannelFiles; // per-channel
LogCallback                                      s_Callback;     // in-app sink

// ---------------------------------------------------------------------------
// Timestamp helpers (called under lock)
// ---------------------------------------------------------------------------

/// Return elapsed milliseconds from program start (for console line format).
int64_t ElapsedMs() noexcept {
    using Clock = std::chrono::steady_clock;
    static const auto kEpoch = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               Clock::now() - kEpoch).count();
}

/// Return an ISO-8601 wall-clock timestamp string for log files.
std::string WallClockTimestamp() {
    const auto now   = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    const auto ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now.time_since_epoch()).count() % 1000;
    std::tm    tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &timeT);
#else
    localtime_r(&timeT, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms;
    return oss.str();
}

/// Open (or return existing) per-channel log file.
/// Precondition: s_Mutex is held, s_FileLoggingActive == true.
std::ofstream& GetChannelFile(const std::string& channel) {
    auto it = s_ChannelFiles.find(channel);
    if (it != s_ChannelFiles.end() && it->second.is_open())
        return it->second;

    // Create subdirectory: Saved/Logs/{Channel}/
    const std::filesystem::path dir =
        std::filesystem::path(s_LogRoot) / channel;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    const std::filesystem::path filePath = dir / (channel + ".log");
    auto& f = s_ChannelFiles[channel];
    f.open(filePath, std::ios::out | std::ios::app);
    if (f.is_open()) {
        f << "--- Log opened: " << WallClockTimestamp()
          << " [channel=" << channel << "] ---\n";
    }
    return f;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool Logger::Init(const std::string& logRoot) {
    std::lock_guard lock(s_Mutex);

    if (s_FileLoggingActive) {
        // Already initialised — close previous session first.
        s_MasterFile.close();
        for (auto& [k, f] : s_ChannelFiles)
            f.close();
        s_ChannelFiles.clear();
    }

    // Ensure the root directory exists.
    s_LogRoot = logRoot;
    std::error_code ec;
    std::filesystem::create_directories(s_LogRoot, ec);
    if (ec) {
        std::cerr << "[Logger] Failed to create log directory: "
                  << s_LogRoot << " (" << ec.message() << ")\n";
        s_FileLoggingActive = false;
        return false;
    }

    // Open the master combined log file.
    const std::filesystem::path masterPath =
        std::filesystem::path(s_LogRoot) / "NovaForge.log";
    s_MasterFile.open(masterPath, std::ios::out | std::ios::app);
    if (!s_MasterFile.is_open()) {
        std::cerr << "[Logger] Failed to open master log: "
                  << masterPath.string() << '\n';
        s_FileLoggingActive = false;
        return false;
    }

    s_FileLoggingActive = true;
    s_MasterFile << "\n=== NovaForge Log Session — "
                 << WallClockTimestamp() << " ===\n\n";
    return true;
}

void Logger::Shutdown() {
    std::lock_guard lock(s_Mutex);
    if (s_FileLoggingActive) {
        const auto ts = WallClockTimestamp();
        s_MasterFile << "\n=== Session ended — " << ts << " ===\n";
        s_MasterFile.close();
        for (auto& [k, f] : s_ChannelFiles) {
            if (f.is_open()) {
                f << "--- Log closed: " << ts << " ---\n";
                f.close();
            }
        }
        s_ChannelFiles.clear();
    }
    s_FileLoggingActive = false;
    s_LogRoot.clear();
}

std::string_view Logger::LevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?????";
}

void Logger::SetMinLevel(LogLevel level) noexcept {
    std::lock_guard lock(s_Mutex);
    s_MinLevel = level;
}

bool Logger::IsFileLoggingActive() noexcept {
    std::lock_guard lock(s_Mutex);
    return s_FileLoggingActive;
}

std::string Logger::GetLogRoot() noexcept {
    std::lock_guard lock(s_Mutex);
    return s_LogRoot;
}

void Logger::SetCallback(LogCallback cb) {
    std::lock_guard lock(s_Mutex);
    s_Callback = std::move(cb);
}

void Logger::Log(LogLevel level, std::string_view channel, std::string_view msg) {
    if (level < s_MinLevel)
        return;

    const auto ms  = ElapsedMs();
    const auto lvl = LevelToString(level);

    // Build the formatted log line once — reused across all sinks.
    std::string line;
    line.reserve(64 + channel.size() + msg.size());
    line += '[';
    line += std::to_string(ms);
    line += "ms][";
    line += lvl;
    line += "][";
    line += channel;
    line += "] ";
    line += msg;

    // ----- Sink 1: Console (stdout / stderr) -----
    {
        auto& out = (level >= LogLevel::Error) ? std::cerr : std::cout;
        std::lock_guard lock(s_Mutex);
        out << line << '\n';
    }

    // ----- Sink 2: VS Output Debug String (Windows only) -----
#ifdef _WIN32
    {
        std::string dbg = line;
        dbg += '\n';
        OutputDebugStringA(dbg.c_str());
    }
#endif

    // ----- Sink 3 & 4: Master log file + per-channel log file -----
    {
        std::lock_guard lock(s_Mutex);
        if (s_FileLoggingActive) {
            const auto ts = WallClockTimestamp();

            // Build the file-format line: "[timestamp][LEVEL][channel] msg"
            std::string fileLine;
            fileLine.reserve(64 + channel.size() + msg.size());
            fileLine += '[';
            fileLine += ts;
            fileLine += "][";
            fileLine += lvl;
            fileLine += "][";
            fileLine += channel;
            fileLine += "] ";
            fileLine += msg;
            fileLine += '\n';

            // Master log
            s_MasterFile << fileLine;
            s_MasterFile.flush();

            // Per-channel log
            const std::string channelStr(channel);
            auto& cf = GetChannelFile(channelStr);
            if (cf.is_open()) {
                cf << fileLine;
                cf.flush();
            }
        }
    }

    // ----- Sink 5: In-app callback (e.g. editor ConsolePanel) -----
    // Copy the callback under the lock, then invoke outside it so the
    // callback can safely call Logger::Log without deadlocking.
    {
        LogCallback cbCopy;
        {
            std::lock_guard lock(s_Mutex);
            cbCopy = s_Callback;
        }
        if (cbCopy) {
            try { cbCopy(line); }
            catch (...) { /* Swallow exceptions from the callback to keep
                             the logger running reliably. */ }
        }
    }

    if (level == LogLevel::Fatal)
        std::abort();
}

} // namespace NF

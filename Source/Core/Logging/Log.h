#pragma once
#include <functional>
#include <string>
#include <string_view>

namespace NF {

/// @brief Severity levels for the logging system, ordered least to most severe.
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/// @brief Callback type for the in-app log sink.
///
/// Receives the fully formatted log line (e.g. "[123ms][INFO ][Editor] msg").
using LogCallback = std::function<void(std::string_view formattedLine)>;

/// @brief Central, thread-safe, multi-sink logging facility for NovaForge.
///
/// Sinks:
///   1. **Console** (stdout / stderr) — always active.
///   2. **Master log file** — `{logRoot}/NovaForge.log` — all channels combined.
///   3. **Per-channel log files** — `{logRoot}/{Channel}/{Channel}.log`.
///   4. **VS Output Debug String** — on Windows, every message is also sent to
///      the Visual Studio Output window via `OutputDebugStringA`.
///   5. **In-app callback** — optional; forwards each formatted line to
///      a user-supplied function (e.g. the editor's ConsolePanel).
///
/// Call @c Init() early in your program (before any Log() calls if you want
/// file output).  If Init() is never called the logger still works as a
/// console-only logger — no files are opened and no crash occurs.
class Logger {
public:
    /// @brief Initialise file sinks.
    ///
    /// Creates (or reuses) the directory @p logRoot and opens the master
    /// log file.  Per-channel subdirectories are created lazily on first
    /// message.
    ///
    /// @param logRoot  Absolute or CWD-relative directory for log files
    ///                 (e.g. "Saved/Logs").
    /// @return True if the master log file was opened successfully.
    static bool Init(const std::string& logRoot);

    /// @brief Flush and close all open log files.
    static void Shutdown();

    /// @brief Emit a log message to all active sinks.
    /// @param level   Severity of the message.
    /// @param channel Subsystem or category name (e.g. "Renderer", "Physics").
    /// @param msg     Human-readable message text.
    static void Log(LogLevel level, std::string_view channel, std::string_view msg);

    /// @brief Set the minimum severity level; messages below it are silently discarded.
    /// @param level The new minimum level.
    static void SetMinLevel(LogLevel level) noexcept;

    /// @brief Return the short label string for a given log level.
    /// @param level The level to stringify.
    static std::string_view LevelToString(LogLevel level) noexcept;

    /// @brief Query whether file sinks have been initialised.
    [[nodiscard]] static bool IsFileLoggingActive() noexcept;

    /// @brief Return the resolved log root directory (empty if not initialised).
    [[nodiscard]] static std::string GetLogRoot() noexcept;

    /// @brief Register an in-app callback that receives every formatted log
    ///        line.  Pass an empty/null callback to remove a previous one.
    ///
    /// The callback is invoked **outside** the logger mutex so it is safe for
    /// the callback to call back into Logger::Log without deadlocking.
    static void SetCallback(LogCallback cb);
};

} // namespace NF

// ---------------------------------------------------------------------------
// Convenience logging macros
// ---------------------------------------------------------------------------
#define NF_LOG_TRACE(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Trace,   (channel), (msg))
#define NF_LOG_DEBUG(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Debug,   (channel), (msg))
#define NF_LOG_INFO(channel, msg)   ::NF::Logger::Log(::NF::LogLevel::Info,    (channel), (msg))
#define NF_LOG_WARN(channel, msg)   ::NF::Logger::Log(::NF::LogLevel::Warning, (channel), (msg))
#define NF_LOG_ERROR(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Error,   (channel), (msg))
#define NF_LOG_FATAL(channel, msg)  ::NF::Logger::Log(::NF::LogLevel::Fatal,   (channel), (msg))

/// @file LogTests.cpp — Unit tests for the Core logging system.
#include <catch2/catch_test_macros.hpp>
#include "Core/Logging/Log.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace NF;

TEST_CASE("LogLevel ordering", "[core][log]") {
    REQUIRE(static_cast<int>(LogLevel::Trace)   < static_cast<int>(LogLevel::Debug));
    REQUIRE(static_cast<int>(LogLevel::Debug)   < static_cast<int>(LogLevel::Info));
    REQUIRE(static_cast<int>(LogLevel::Info)    < static_cast<int>(LogLevel::Warning));
    REQUIRE(static_cast<int>(LogLevel::Warning) < static_cast<int>(LogLevel::Error));
    REQUIRE(static_cast<int>(LogLevel::Error)   < static_cast<int>(LogLevel::Fatal));
}

TEST_CASE("Logger::Log does not throw", "[core][log]") {
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Info,    "Test", "info message"));
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Warning, "Test", "warning message"));
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Error,   "Test", "error message"));
}

TEST_CASE("NF_LOG macros compile and run without throwing", "[core][log]") {
    REQUIRE_NOTHROW(NF_LOG_INFO("Test", "macro info"));
    REQUIRE_NOTHROW(NF_LOG_WARN("Test", "macro warn"));
    REQUIRE_NOTHROW(NF_LOG_ERROR("Test", "macro error"));
}

// ---------------------------------------------------------------------------
// File-sink tests
// ---------------------------------------------------------------------------

/// Helper: read an entire text file into a string.
static std::string ReadFileContents(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    return std::string{std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>()};
}

TEST_CASE("Logger::Init creates log directory and master file", "[core][log][file]") {
    const std::string logDir = "/tmp/nf_log_test_init";
    std::filesystem::remove_all(logDir);

    REQUIRE(Logger::Init(logDir));
    REQUIRE(Logger::IsFileLoggingActive());
    REQUIRE(Logger::GetLogRoot() == logDir);
    REQUIRE(std::filesystem::exists(logDir));
    REQUIRE(std::filesystem::exists(std::filesystem::path(logDir) / "NovaForge.log"));

    Logger::Shutdown();
    REQUIRE_FALSE(Logger::IsFileLoggingActive());
    std::filesystem::remove_all(logDir);
}

TEST_CASE("Logger writes to master log and per-channel log", "[core][log][file]") {
    const std::string logDir = "/tmp/nf_log_test_channel";
    std::filesystem::remove_all(logDir);

    REQUIRE(Logger::Init(logDir));

    Logger::Log(LogLevel::Info, "Renderer", "Frame 42 completed");
    Logger::Log(LogLevel::Warning, "Physics", "Collision overlap");
    Logger::Log(LogLevel::Error, "Audio", "Buffer underrun");

    Logger::Shutdown();

    // Master log should contain all three messages
    const auto masterContent = ReadFileContents(
        std::filesystem::path(logDir) / "NovaForge.log");
    REQUIRE(masterContent.find("Frame 42 completed") != std::string::npos);
    REQUIRE(masterContent.find("Collision overlap")  != std::string::npos);
    REQUIRE(masterContent.find("Buffer underrun")    != std::string::npos);

    // Per-channel subdirectories should exist
    REQUIRE(std::filesystem::exists(
        std::filesystem::path(logDir) / "Renderer" / "Renderer.log"));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path(logDir) / "Physics" / "Physics.log"));
    REQUIRE(std::filesystem::exists(
        std::filesystem::path(logDir) / "Audio" / "Audio.log"));

    // Per-channel files should contain only their own messages
    const auto rendererLog = ReadFileContents(
        std::filesystem::path(logDir) / "Renderer" / "Renderer.log");
    REQUIRE(rendererLog.find("Frame 42 completed") != std::string::npos);
    REQUIRE(rendererLog.find("Collision overlap")  == std::string::npos);

    const auto physicsLog = ReadFileContents(
        std::filesystem::path(logDir) / "Physics" / "Physics.log");
    REQUIRE(physicsLog.find("Collision overlap") != std::string::npos);
    REQUIRE(physicsLog.find("Frame 42 completed") == std::string::npos);

    std::filesystem::remove_all(logDir);
}

TEST_CASE("Logger works without Init (console-only mode)", "[core][log][file]") {
    // Ensure no file logging is active
    Logger::Shutdown();
    REQUIRE_FALSE(Logger::IsFileLoggingActive());

    // Should not throw even without Init
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Info, "Test", "console-only message"));
}

TEST_CASE("Logger level strings are correct", "[core][log]") {
    REQUIRE(Logger::LevelToString(LogLevel::Trace)   == "TRACE");
    REQUIRE(Logger::LevelToString(LogLevel::Debug)   == "DEBUG");
    REQUIRE(Logger::LevelToString(LogLevel::Info)     == "INFO ");
    REQUIRE(Logger::LevelToString(LogLevel::Warning)  == "WARN ");
    REQUIRE(Logger::LevelToString(LogLevel::Error)    == "ERROR");
    REQUIRE(Logger::LevelToString(LogLevel::Fatal)    == "FATAL");
}

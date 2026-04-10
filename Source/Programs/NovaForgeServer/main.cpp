// Entry point for the NovaForge Dedicated Server.
//
// The dedicated server runs headless (no window, no renderer) and hosts
// a game session for remote clients.  It uses the same Logger, game
// systems, and networking stack as the game client but with no UI layer.
//
// Compile with: cmake -B build -DNF_BUILD_SERVER=ON && cmake --build build
// Run with:     NovaForgeServer [--world <worldName>] [--port <port>]
#include "Core/Logging/Log.h"
#include "Core/Config/ProjectManifest.h"
#include "Game/App/Orchestrator.h"
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>
#include <cstdlib>

// Dedicated server does not suppress the console window — operators need it.

namespace {

struct ServerConfig {
    std::string world{"DevWorld"};
    int         port{27015};
    int         maxPlayers{20};
    int         tickRate{60};
};

void PrintUsage(const char* argv0)
{
    NF::Logger::Log(NF::LogLevel::Info, "Server",
        std::string("Usage: ") + argv0 +
        " [--world <name>] [--port <n>] [--max-players <n>] [--tick-rate <n>]");
}

ServerConfig ParseArgs(int argc, char* argv[])
{
    ServerConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        auto nextStr = [&]() -> std::string {
            return (i + 1 < argc) ? std::string(argv[++i]) : std::string{};
        };
        auto nextInt = [&](int fallback) -> int {
            if (i + 1 >= argc) return fallback;
            return std::atoi(argv[++i]);
        };
        if      (arg == "--world")       cfg.world      = nextStr();
        else if (arg == "--port")        cfg.port       = nextInt(cfg.port);
        else if (arg == "--max-players") cfg.maxPlayers = nextInt(cfg.maxPlayers);
        else if (arg == "--tick-rate")   cfg.tickRate   = nextInt(cfg.tickRate);
        else if (arg == "--help" || arg == "-h") {
            // Will be printed after logger init
        }
    }
    return cfg;
}

} // namespace

int main(int argc, char* argv[])
{
    // Set the working directory to the parent of the executable's directory
    // so that Content/, Config/, and Saved/ resolve correctly.
    {
        std::error_code ec;
        auto exeDir = std::filesystem::weakly_canonical(
            std::filesystem::path(argv[0]).parent_path(), ec);
        if (!ec && !exeDir.empty()) {
            auto parentDir = exeDir.parent_path();
            if (std::filesystem::exists(parentDir / "Config" / "novaforge.project.json", ec))
                std::filesystem::current_path(parentDir, ec);
        }
    }

    NF::Logger::Init("Saved/Logs");
    NF::Logger::Log(NF::LogLevel::Info, "Server", "=== NovaForge Dedicated Server ===");

    if (argc < 2) {
        PrintUsage(argv[0]);
    }

    const ServerConfig cfg = ParseArgs(argc, argv);

    NF::Logger::Log(NF::LogLevel::Info, "Server", "  World      : " + cfg.world);
    NF::Logger::Log(NF::LogLevel::Info, "Server", "  Port       : " + std::to_string(cfg.port));
    NF::Logger::Log(NF::LogLevel::Info, "Server", "  Max players: " + std::to_string(cfg.maxPlayers));
    NF::Logger::Log(NF::LogLevel::Info, "Server", "  Tick rate  : " + std::to_string(cfg.tickRate));

    // -------------------------------------------------------------------------
    // Bootstrap: Orchestrator in Dedicated mode.
    // -------------------------------------------------------------------------

    NF::Game::NetParams params;
    params.Mode       = NF::Game::NetMode::Dedicated;
    params.Port       = static_cast<uint16_t>(cfg.port);
    params.PlayerName = "Server";

    NF::Game::Orchestrator orchestrator;
    if (!orchestrator.Init(nullptr /* no renderer */, params)) {
        NF::Logger::Log(NF::LogLevel::Error, "Server", "Orchestrator init failed — aborting");
        NF::Logger::Shutdown();
        return 1;
    }

    NF::Logger::Log(NF::LogLevel::Info, "Server",
                    "Server ready — listening on port " + std::to_string(cfg.port)
                    + "  (tick rate " + std::to_string(cfg.tickRate) + " Hz)");

    // -------------------------------------------------------------------------
    // Main tick loop — runs until stdin is closed or Ctrl-C is received.
    // -------------------------------------------------------------------------

    const float dt = 1.f / static_cast<float>(cfg.tickRate);
    const auto tickDuration = std::chrono::microseconds(
        static_cast<long long>(1'000'000.0 / cfg.tickRate));

    auto nextTick = std::chrono::steady_clock::now();
    uint64_t totalTicks = 0;

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        if (now < nextTick) {
            std::this_thread::sleep_until(nextTick);
        }
        nextTick += tickDuration;

        orchestrator.Tick(dt);
        ++totalTicks;

        // Log a heartbeat every 5 seconds.
        if (totalTicks % (static_cast<uint64_t>(cfg.tickRate) * 5) == 0) {
            const NF::Game::GameServer* srv = orchestrator.GetServer();
            const size_t clients = srv ? srv->ClientCount() : 0;
            NF::Logger::Log(NF::LogLevel::Debug, "Server",
                            "Tick " + std::to_string(totalTicks)
                            + "  clients=" + std::to_string(clients));
        }
    }

    // Unreachable in normal operation — process is killed by the OS.
    orchestrator.Shutdown();
    NF::Logger::Log(NF::LogLevel::Info, "Server", "NovaForge Server exiting.");
    NF::Logger::Shutdown();
    return 0;
}

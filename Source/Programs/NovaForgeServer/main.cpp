// Entry point for the NovaForge Dedicated Server.
//
// The dedicated server runs headless (no window, no renderer) and hosts
// a game session for remote clients.  It uses the same Logger, game
// systems, and networking stack as the game client but with no UI layer.
//
// Compile with: cmake -B build -DNF_BUILD_SERVER=ON && cmake --build build
// Run with:     NovaForgeServer [--world <worldName>] [--port <port>]
#include "Core/Logging/Log.h"
#include <filesystem>
#include <string>
#include <string_view>
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

    // TODO (Phase 6 implementation): replace this stub with:
    //   NF::Game::ServerApp app;
    //   app.Init(cfg.world, cfg.port, cfg.maxPlayers, cfg.tickRate);
    //   app.Run();
    //   app.Shutdown();
    //
    // For now, the server validates that all systems compile and link correctly.

    NF::Logger::Log(NF::LogLevel::Info, "Server",
        "Server stub: no networking loop yet (Phase 6 implementation pending).");

    NF::Logger::Log(NF::LogLevel::Info, "Server", "NovaForge Server exiting.");
    NF::Logger::Shutdown();
    return 0;
}

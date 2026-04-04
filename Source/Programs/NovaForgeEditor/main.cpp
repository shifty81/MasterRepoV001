// Entry point for the NovaForge Editor
#include "Editor/Application/EditorApp.h"
#include "Core/Logging/Log.h"
#include <filesystem>

int main(int argc, char* argv[])
{
    // Set the working directory to the parent of the executable's directory
    // so that Content/, Config/, and Saved/ resolve correctly when the
    // executable lives inside Nova_<version>/bin/.
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
    NF::Logger::Log(NF::LogLevel::Info, "Editor", "Starting NovaForge Editor");
    NF::Editor::EditorApp app;
    if (!app.Init())
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Editor", "Failed to initialise editor");
        return 1;
    }
    app.Run();
    app.Shutdown();
    NF::Logger::Shutdown();
    return 0;
}

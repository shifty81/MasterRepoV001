add_library(NFEngine STATIC
    Application/EngineLoop.cpp
    ECS/World.cpp
    World/Level.cpp
    Assets/AssetManager.cpp
    AI/BehaviorTree.cpp
    AI/AISystem.cpp
)

add_library(NF::Engine ALIAS NFEngine)

target_include_directories(NFEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(NFEngine PUBLIC NF::Core NF::NodeGraph)

message(STATUS "  NF::Engine configured")

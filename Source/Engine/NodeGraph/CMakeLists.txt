add_library(NFNodeGraph STATIC
    GraphVM.cpp
)

add_library(NF::NodeGraph ALIAS NFNodeGraph)

target_include_directories(NFNodeGraph PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(NFNodeGraph PUBLIC NF::Core)

message(STATUS "  NF::NodeGraph configured")

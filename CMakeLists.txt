cmake_minimum_required(VERSION 3.20)
project(NovaForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories — binaries land in Nova_<version>/ at the repo root.
set(NF_OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/Nova_${PROJECT_VERSION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/lib)

# ---------------------------------------------------------------------------
# Mirror project folders into the output directory so the executables can
# find Content/, Config/, and Saved/ relative to Nova_<version>/.
# During development we create symbolic links so edits are reflected
# immediately without a copy step.
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY "${NF_OUTPUT_ROOT}/Saved/Logs")

# Symlink Content/ and Config/ into the output tree.
foreach(_dir Content Config)
    if(NOT EXISTS "${NF_OUTPUT_ROOT}/${_dir}")
        file(CREATE_LINK
             "${CMAKE_SOURCE_DIR}/${_dir}"
             "${NF_OUTPUT_ROOT}/${_dir}"
             SYMBOLIC)
        message(STATUS "  Linked ${_dir}/ -> Nova_${PROJECT_VERSION}/${_dir}/")
    endif()
endforeach()

# Build options
option(NF_BUILD_EDITOR "Build NovaForge Editor" ON)
option(NF_BUILD_GAME   "Build NovaForge Game Runtime" ON)
option(NF_BUILD_TESTS  "Build test suite" OFF)

# Modules (static libraries)
add_subdirectory(Source/Core)
add_subdirectory(Source/Engine)
add_subdirectory(Source/Renderer)
add_subdirectory(Source/Physics)
add_subdirectory(Source/Audio)
add_subdirectory(Source/Animation)
add_subdirectory(Source/Input)
add_subdirectory(Source/Networking)
add_subdirectory(Source/UI)
add_subdirectory(Source/Game)

if(NF_BUILD_EDITOR)
    add_subdirectory(Source/Editor)
endif()

# Executables
add_subdirectory(Source/Programs)

if(NF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

message(STATUS "")
message(STATUS "=== NovaForge ${PROJECT_VERSION} ===")
message(STATUS "  Build Editor:  ${NF_BUILD_EDITOR}")
message(STATUS "  Build Game:    ${NF_BUILD_GAME}")
message(STATUS "  Build Tests:   ${NF_BUILD_TESTS}")
message(STATUS "")

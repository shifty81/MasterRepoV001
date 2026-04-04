cmake_minimum_required(VERSION 3.20)
project(NovaForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories — binaries land in Nova_<version>/ at the repo root.
# The per-config variants (DEBUG, RELEASE, …) ensure that multi-config
# generators (Visual Studio) do *not* append a config subdirectory.
# This means single-config and multi-config builds produce the same flat
# layout, and Launch Game can always find NovaForgeGame.exe beside the editor.
set(NF_OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/Nova_${PROJECT_VERSION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${NF_OUTPUT_ROOT}/lib)

foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${NF_OUTPUT_ROOT}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg} ${NF_OUTPUT_ROOT}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg} ${NF_OUTPUT_ROOT}/lib)
endforeach()

# ---------------------------------------------------------------------------
# Mirror project folders into the output directory so the executables can
# find Content/, Config/, and Saved/ relative to Nova_<version>/.
# During development we create symbolic links so edits are reflected
# immediately without a copy step.
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY "${NF_OUTPUT_ROOT}/Saved/Logs")

# Symlink Content/ and Config/ into the output tree.
# On Windows, symbolic links require Developer Mode or elevated privileges.
# If the symlink fails, fall back to a directory junction (mklink /J) which
# works without elevation and still reflects edits immediately.
foreach(_dir Content Config)
    if(NOT EXISTS "${NF_OUTPUT_ROOT}/${_dir}")
        file(CREATE_LINK
             "${CMAKE_SOURCE_DIR}/${_dir}"
             "${NF_OUTPUT_ROOT}/${_dir}"
             SYMBOLIC
             RESULT _link_result)
        if(_link_result EQUAL 0)
            message(STATUS "  Linked ${_dir}/ -> Nova_${PROJECT_VERSION}/${_dir}/")
        elseif(WIN32)
            # Fall back to a directory junction (no elevation required).
            execute_process(
                COMMAND cmd /c mklink /J
                        "${NF_OUTPUT_ROOT}/${_dir}"
                        "${CMAKE_SOURCE_DIR}/${_dir}"
                RESULT_VARIABLE _junc_result)
            if(_junc_result EQUAL 0)
                message(STATUS "  Junction ${_dir}/ -> Nova_${PROJECT_VERSION}/${_dir}/")
            else()
                message(WARNING "Could not link or junction ${_dir}/: ${_junc_result}")
            endif()
        else()
            message(WARNING "Could not create symlink for ${_dir}/: ${_link_result}")
        endif()
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

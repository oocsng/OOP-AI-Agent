cmake_minimum_required(VERSION 3.20)
project(OOP_AI_Agents_2026 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Auto-detect MSYS2 UCRT64 dependencies on Windows
if(WIN32)
    if(EXISTS "C:/msys64/ucrt64")
        list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
        if(NOT DEFINED OPENSSL_ROOT_DIR)
            set(OPENSSL_ROOT_DIR "C:/msys64/ucrt64")
        endif()
    endif()
endif()

# Dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core framework library sources
file(GLOB_RECURSE CORE_SOURCES
    "src/agent/*.cpp"
    "src/client/*.cpp"
    "src/harness/*.cpp"
    "src/tools/*.cpp"
)

# Executable 1: Main Application
add_executable(agent_app src/main.cpp ${CORE_SOURCES})

target_link_libraries(agent_app PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

# Executable 2: Benchmark Evaluation Runner
add_executable(run_eval benchmark/run_eval.cpp ${CORE_SOURCES})

target_link_libraries(run_eval PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(agent_app PRIVATE stdc++exp)
    target_link_libraries(run_eval PRIVATE stdc++exp)
    target_compile_options(agent_app PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
    target_compile_options(run_eval PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
elseif(MSVC)
    target_compile_options(agent_app PRIVATE /W4 /std:c++latest)
    target_compile_options(run_eval PRIVATE /W4 /std:c++latest)
endif()

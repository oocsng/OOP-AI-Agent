cmake_minimum_required(VERSION 3.21)
project(OOP_AI_Agents_2026 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Auto-detect MSYS2 UCRT64 dependencies on Windows
if(WIN32)
    if(DEFINED ENV{MINGW_PREFIX} AND EXISTS "$ENV{MINGW_PREFIX}")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{MINGW_PREFIX}")
        if(NOT DEFINED OPENSSL_ROOT_DIR)
            set(OPENSSL_ROOT_DIR "$ENV{MINGW_PREFIX}")
        endif()
    elseif(EXISTS "C:/msys64/ucrt64")
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

# Core framework library sources
file(GLOB_RECURSE CORE_SOURCES
    "src/agent/*.cpp"
    "src/client/*.cpp"
    "src/harness/*.cpp"
    "src/tools/*.cpp"
)

# Core static library
add_library(oop_agent_core STATIC ${CORE_SOURCES})

target_include_directories(oop_agent_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(oop_agent_core PUBLIC
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite3::SQLite3
    gdi32
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(oop_agent_core PUBLIC stdc++exp)
    target_compile_options(oop_agent_core PUBLIC -Wall -Wextra -Wpedantic -std=c++26)
elseif(MSVC)
    target_compile_options(oop_agent_core PUBLIC /W4 /std:c++latest)
endif()

# Executable 1: Main Application
add_executable(agent_app src/main.cpp)
target_link_libraries(agent_app PRIVATE oop_agent_core)

# Executable 2: Benchmark Evaluation Runner
add_executable(run_eval benchmark/run_eval.cpp)
target_link_libraries(run_eval PRIVATE oop_agent_core)

# Executable 3: Browser Automation Demo Runner
add_executable(run_browser_demo benchmark/run_browser_demo.cpp)
target_link_libraries(run_browser_demo PRIVATE oop_agent_core)

# Enable Testing & Register Unit Test Executables with CTest
enable_testing()

set(TEST_NAMES
    test_client
    test_tools
    test_loop_detector
    test_skill_loader
    test_agent_loop
    test_evaluators
    test_bonus_gui
    test_bonus_vector_memory
    test_bonus_multi_agent
)

foreach(test_name ${TEST_NAMES})
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE oop_agent_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

add_custom_target(unit_tests DEPENDS ${TEST_NAMES})

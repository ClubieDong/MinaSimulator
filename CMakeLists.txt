cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE vcpkg/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(mina_sim LANGUAGES CXX)

# ========== Enable link time optimization if supported ==========
include(CheckIPOSupported)
check_ipo_supported(RESULT CHECK_IPO_RESULT)
if(CHECK_IPO_RESULT)
    message("Link time optimization is enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# ========== Compiler & linker options ==========
# Enable the highest level of warning
set(WARNING_OPTIONS
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>: -Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:                 /Wall                   >
)

# ========== Third-party libraries ==========
# pthread
find_package(Threads REQUIRED)
list(APPEND THRID_PARTY_LIBRARIES Threads::Threads)
# nlohmann/json
find_package(nlohmann_json CONFIG REQUIRED)
list(APPEND THRID_PARTY_LIBRARIES nlohmann_json nlohmann_json::nlohmann_json)

# ========== Targets ==========
# Core library
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_OPTIONS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${THRID_PARTY_LIBRARIES})

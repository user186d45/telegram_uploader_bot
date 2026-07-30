cmake_minimum_required(VERSION 3.10)
project(telegram_uploader_bot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Max optimization for speed
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -march=native -flto -funroll-loops")

# Find required packages
find_package(Threads REQUIRED)

# Find TgBot library
find_path(TGBOT_INCLUDE_DIR NAMES tgbot/tgbot.h)
find_library(TGBOT_LIBRARY NAMES tgbot TgBot)

if(NOT TGBOT_INCLUDE_DIR OR NOT TGBOT_LIBRARY)
    message(WARNING "TgBot library not found in standard paths. Please specify TGBOT_INCLUDE_DIR and TGBOT_LIBRARY.")
    set(TGBOT_INCLUDE_DIR "/usr/local/include")
    set(TGBOT_LIBRARY "/usr/local/lib/libtgbot.a")
endif()

# Find cJSON library
find_path(CJSON_INCLUDE_DIR NAMES cjson/cJSON.h)
find_library(CJSON_LIBRARY NAMES cjson)

# Find SQLite3 library
find_package(SQLite3 REQUIRED)

# Find PCRE2
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCRE2 REQUIRED libpcre2-8)

# Find OpenSSL (required by TgBot)
find_package(OpenSSL REQUIRED)

# Find Boost (required by TgBot)
find_package(Boost REQUIRED COMPONENTS system)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${TGBOT_INCLUDE_DIR}
    ${CJSON_INCLUDE_DIR}
    ${PCRE2_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${TGBOT_LIBRARY}
    ${CJSON_LIBRARY}
)

# Source files
set(SOURCES
    src/main.cpp
    src/bot.cpp
    src/structs.cpp
    src/cjson.cpp
    src/passgen.cpp
    src/sqlite.cpp
    src/log.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${TGBOT_LIBRARY}
    ${CJSON_LIBRARY}
    ${PCRE2_LIBRARIES}
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    Threads::Threads
    pthread
    stdc++fs
)

# Suppress Boost deprecation warning (TgBot uses old boost/bind.hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_BIND_GLOBAL_PLACEHOLDERS)

# Add compile definitions
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} rt)
endif()

# Output directory for build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "telegram_uploader_bot CMake configuration complete")
message(STATUS "TgBot include: ${TGBOT_INCLUDE_DIR}")
message(STATUS "TgBot library: ${TGBOT_LIBRARY}")

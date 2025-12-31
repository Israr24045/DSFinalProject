cmake_minimum_required(VERSION 3.10)
project(SeasonCanvas)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads library (required by cpp-httplib)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    backend/main.cpp
    backend/server.cpp
    backend/database.cpp
    backend/btree.cpp
    backend/canvas.cpp
    backend/snapshot.cpp
    backend/video_export.cpp
    backend/sha256.cpp
)

# Create executable
add_executable(season_canvas ${SOURCES})

# Link threads
target_link_libraries(season_canvas PRIVATE Threads::Threads)

# Include directories
target_include_directories(season_canvas PRIVATE backend)

# Enable warnings
if(MSVC)
    target_compile_options(season_canvas PRIVATE /W4)
else()
    target_compile_options(season_canvas PRIVATE -Wall -Wextra -pedantic)
endif()
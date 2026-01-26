cmake_minimum_required(VERSION 3.16)
project(VoxelWorld VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for build configuration
option(BUILD_SERVER "Build the dedicated server" ON)
option(BUILD_CLIENT "Build the game client" ON)
option(BUILD_SHARED "Build shared library" ON)

# Dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Network)
if(BUILD_CLIENT)
    find_package(Qt6 REQUIRED COMPONENTS Gui Widgets ShaderTools)
endif()

# Our libraries
add_subdirectory(shared)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
cmake_minimum_required(VERSION 3.10)
project( Nmbr9 )

# C++ standard version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The version number.
set (Nmbr9_VERSION_MAJOR 0)
set (Nmbr9_VERSION_MINOR 1)
set (Nmbr9_VERSION_PATCH 0)

add_subdirectory (src)

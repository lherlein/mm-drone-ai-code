cmake_minimum_required(VERSION 3.16)
project(mm-drone-ai VERSION 1.0.0)

# Options for building different components
option(BUILD_ACU "Build Air Control Unit" OFF)
option(BUILD_GCU "Build Ground Control Unit" OFF)
option(BUILD_TESTS "Build test suite" OFF)

# Common compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencies
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Add common library
add_subdirectory(common)

# Conditionally add ACU and GCU
if(BUILD_ACU)
    add_subdirectory(acu)
endif()

if(BUILD_GCU)
    add_subdirectory(gcu)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif() 
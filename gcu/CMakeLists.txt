cmake_minimum_required(VERSION 3.10)
project(DroneGCU VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# Header files
file(GLOB_RECURSE HEADERS
    "include/*.hpp"
)

# Create executable
add_executable(drone_gcu ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(drone_gcu
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        pthread
)

# Installation
install(TARGETS drone_gcu
    RUNTIME DESTINATION bin
)

# Configuration files
install(FILES config/gcu_config.yaml
    DESTINATION etc/mm-drone) 
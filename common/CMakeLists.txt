cmake_minimum_required(VERSION 3.10)

# Common library
add_library(common
    src/protocol/Packet.cpp
)

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(common
    PUBLIC
        cxx_std_17
)

# Install headers
install(DIRECTORY include/
    DESTINATION include
target_link_libraries(common PUBLIC
    Threads::Threads
    Boost::system
) 
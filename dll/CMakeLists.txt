cmake_minimum_required(VERSION 3.10)
project(xit1299 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared library (DLL on Windows, .so on Linux, .dylib on macOS)
add_library(xit1299 SHARED xit1299.cpp)

target_compile_definitions(xit1299 PRIVATE XIT1299_EXPORTS)

if(WIN32)
    target_link_libraries(xit1299 PRIVATE winhttp crypt32 user32 advapi32)
    # Enable static runtime for portability
    set_property(TARGET xit1299 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
else()
    find_package(CURL REQUIRED)
    target_link_libraries(xit1299 PRIVATE CURL::libcurl)
    # Set visibility
    target_compile_options(xit1299 PRIVATE -fvisibility=hidden)
endif()

# Install
install(TARGETS xit1299 DESTINATION lib)
install(FILES xit1299.h DESTINATION include)

# Also build a simple test executable
add_executable(xit1299_test test_main.cpp)
target_link_libraries(xit1299_test PRIVATE xit1299)
if(NOT WIN32)
    target_link_libraries(xit1299_test PRIVATE curl)
endif()

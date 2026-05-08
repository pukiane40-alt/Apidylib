cmake_minimum_required(VERSION 3.14)
project(xit1299 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Shared library ─────────────────────────────────────────────
add_library(xit1299 SHARED xit1299.cpp)

target_compile_definitions(xit1299 PRIVATE XIT1299_EXPORTS)

if(WIN32)
    # Windows: WinHTTP + GDI + User32
    target_link_libraries(xit1299 PRIVATE
        winhttp wininet comctl32 user32 gdi32 advapi32)

    # Static runtime for portability (no MSVC redistributable needed)
    set_property(TARGET xit1299 PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Hide symbols not explicitly exported
    target_compile_options(xit1299 PRIVATE /W3)

else()
    # Linux / macOS: libcurl
    find_package(CURL REQUIRED)
    target_link_libraries(xit1299 PRIVATE CURL::libcurl)
    target_compile_options(xit1299 PRIVATE
        -Wall -Wextra -fvisibility=hidden)
endif()

# ── Install ─────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS xit1299
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll on Windows
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so on Linux
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})  # .lib on Windows
install(FILES xit1299.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# ── Example / test executable ───────────────────────────────────
add_executable(xit1299_test test_main.cpp)
target_link_libraries(xit1299_test PRIVATE xit1299)

if(NOT WIN32)
    target_link_libraries(xit1299_test PRIVATE CURL::libcurl)
endif()

message(STATUS "=== XIT1299 build config ===")
message(STATUS "  Platform : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Build    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

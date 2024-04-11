cmake_minimum_required(VERSION 3.25)
project(mate)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_CLIENT "Build Client" OFF)
if (APPLE)
    set(BUILD_CLIENT ON)
endif()

option(BUILD_SERVER "Build Server" OFF)
if (UNIX AND NOT APPLE)
    set(BUILD_SERVER ON)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

include_directories(${GLIB_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS} ${GSTREAMER_LIBRARY_DIRS})

if (BUILD_CLIENT)
    enable_language(Swift)

    add_executable(client src/client/client.cpp)

    add_subdirectory(lib/glfw)
    find_package(OpenGL REQUIRED)

    include_directories(lib/glfw/include)

    file(GLOB_RECURSE IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/*.cpp)
    add_library(imgui ${IMGUI_SOURCES})
    include_directories(lib/imgui)

    add_library(photogrammetry src/client/photogrammetry.swift)

    pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
    target_include_directories(client PRIVATE ${GSTREAMER_APP_INCLUDE_DIRS} src)
    target_link_directories(client PRIVATE ${GSTREAMER_APP_LIBRARY})
    target_link_libraries(client PRIVATE OpenGL::GL ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} glfw imgui photogrammetry)
endif()

if (BUILD_SERVER)
    add_executable(src/server/server.cpp)
    target_include_directories(server PRIVATE src)
    target_link_libraries(server PRIVATE ${GSTREAMER_LIBRARIES} i2c)
endif()


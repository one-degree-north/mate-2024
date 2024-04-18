cmake_minimum_required(VERSION 3.25)
project(mate LANGUAGES C CXX Swift)

set(CMAKE_CXX_STANDARD 17)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

# glfw
add_subdirectory(lib/glfw)
find_package(OpenGL REQUIRED)
include_directories(lib/glfw/include)

# imgui
file(GLOB_RECURSE IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/*.cpp)
add_library(imgui ${IMGUI_SOURCES})
include_directories(lib/imgui)

# implot
file(GLOB_RECURSE IMPLOT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/implot/*.cpp)
add_library(implot ${IMPLOT_SOURCES})
include_directories(lib/implot)

# pigpio
file(GLOB_RECURSE PIGPIO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/pigpio/*.c)
add_library(pigpio ${PIGPIO_SOURCES})
include_directories(lib/pigpio)

# nfd
add_subdirectory(lib/nativefiledialog-extended)
include_directories(lib/nativefiledialog-extended/src/include)

# photogrammetry
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/photogrammetry-swift.h
        COMMAND ${CMAKE_Swift_COMPILER} -frontend -typecheck
            ${CMAKE_CURRENT_SOURCE_DIR}/src/photogrammetry.swift
            -sdk ${CMAKE_OSX_SYSROOT}
            -module-name PhotogrammetrySwift
            -cxx-interoperability-mode=default
            -emit-clang-header-path ${CMAKE_CURRENT_BINARY_DIR}/include/photogrammetry-swift.h
        DEPENDS src/photogrammetry.swift
        COMMENT "Generating Swift header"
)
add_custom_target(photogrammetry_header DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/photogrammetry-swift.h)

add_library(photogrammetry STATIC src/photogrammetry.swift)
add_dependencies(photogrammetry photogrammetry_header)
set_target_properties(photogrammetry PROPERTIES Swift_MODULE_NAME PhotogrammetrySwift)

add_executable(client
    src/main.cpp
    src/camera_stream.cpp
    src/controls.cpp
    src/photogrammetry.cpp
    src/pi.cpp
)
target_include_directories(client PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(client PUBLIC OpenGL::GL ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} glfw imgui implot nfd pigpio photogrammetry)

cmake_minimum_required(VERSION 3.21)


include("cmake/HunterGate.cmake")

# HUNTER (https://github.com/ruslo/hunter/releases)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.298.tar.gz"
    SHA1 "6386836a2f0eece999f47f79ead46865261ee4ba"
)

project(navipoc)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(glfw)
find_package(glfw3 REQUIRED)

hunter_add_package(glm)
find_package(glm REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(vendor/earcut.hpp-0.12.4)
add_subdirectory(vendor/shapelib-1.5.0)
add_subdirectory(src/map_compiler)
add_subdirectory(src/dear_imgui)
add_subdirectory(src/glad)
add_subdirectory(src/gg)
add_subdirectory(src/common)
add_subdirectory(src/render_lib)
add_subdirectory(src/render_demo)



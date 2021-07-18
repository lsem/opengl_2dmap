cmake_minimum_required(VERSION 2.8)


include("cmake/HunterGate.cmake")

# HUNTER (https://github.com/ruslo/hunter/releases)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.298.tar.gz"
    SHA1 "6386836a2f0eece999f47f79ead46865261ee4ba"
)


project(red_triangle)

#find_package(OpenGL REQUIRED)

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


# gg
set(SOURCE_FILES "")
add_library(gg "gg.cpp" "include/gg.h")
set_target_properties(gg PROPERTIES CXX_STANDARD "17")
target_include_directories(gg PUBLIC "include")


# red_triangle
set(SOURCE_FILES main.cpp glad.c)
add_executable(red_triangle ${SOURCE_FILES})
target_include_directories(red_triangle PRIVATE "include/")
set_target_properties(red_triangle PROPERTIES CXX_STANDARD "17")
target_link_libraries(red_triangle PRIVATE  fmt::fmt glfw glm gg)


# gg_tests
add_executable(gg_tests "gg_tests_main.cpp" "gg_tests.cpp")
set_target_properties(gg_tests PROPERTIES CXX_STANDARD "17")
target_link_libraries(gg_tests PRIVATE gg GTest::gtest)

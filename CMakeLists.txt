cmake_minimum_required(VERSION 2.8)


include("cmake/HunterGate.cmake")

# HUNTER (https://github.com/ruslo/hunter/releases)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.298.tar.gz"
    SHA1 "6386836a2f0eece999f47f79ead46865261ee4ba"
)


project(red_triangle)

#find_package(OpenGL REQUIRED)

hunter_add_package(glfw)
find_package(glfw3 REQUIRED)

hunter_add_package(glm)
find_package(glm REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

set(SOURCE_FILES main.cpp glad.c)
add_executable(red_triangle ${SOURCE_FILES})

target_include_directories(red_triangle PRIVATE "include/")

set_target_properties(red_triangle PROPERTIES CXX_STANDARD "17")

target_link_libraries(red_triangle PRIVATE  fmt::fmt glfw glm )

# target_link_libraries(red_triangle
#   OpenGL::GL)
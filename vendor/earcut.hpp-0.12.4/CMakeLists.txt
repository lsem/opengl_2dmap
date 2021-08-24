cmake_minimum_required(VERSION 3.2)
project(earcut_hpp)

option(EARCUT_BUILD_TESTS "Build the earcut test program" OFF)
option(EARCUT_WARNING_IS_ERROR "Treat warnings as errors" OFF)

add_library(mapbox_earcut INTERFACE)
target_include_directories(mapbox_earcut INTERFACE include)


# # setup compiler flags for earcut
# set(CMAKE_CXX_STANDARD 11)
# if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang$" OR CMAKE_COMPILER_IS_GNUCXX)
#     set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")
#     add_compile_options("-pipe" "-Wall" "-Wextra" "-Wconversion" "-Wpedantic")
#     if (EARCUT_WARNING_IS_ERROR)
#         add_compile_options("-Werror")
#     endif()
#     # optional: -march=native (builds with the optimizations available on the build machine (only for local use!))
# elseif(MSVC)
#     if (EARCUT_WARNING_IS_ERROR)
#         add_compile_options("/WX")
#     endif()
# endif()

# # earcut.hpp
# include_directories("include")

# file(GLOB FIXTURE_SOURCE_FILES test/fixtures/*.cpp test/fixtures/*.hpp)
# source_group(fixtures FILES ${FIXTURE_SOURCE_FILES})
# if (MSVC)
#     set_source_files_properties(${FIXTURE_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "/Od")
# endif()

# file(GLOB COMPARISON_SOURCE_FILES test/comparison/*.cpp test/comparison/*.hpp)
# source_group(comparison FILES ${COMPARISON_SOURCE_FILES})

# file(GLOB LIBTESS2_SOURCE_FILES test/comparison/libtess2/*.c test/comparison/libtess2/*.h)
# source_group(comparison\\libtess2 FILES ${LIBTESS2_SOURCE_FILES})

# if (MSVC)
#     set_source_files_properties(${LIBTESS2_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267")
# else()
#     set_source_files_properties(${LIBTESS2_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-w")
# endif()

# set(COMMON_SOURCE_FILES ${LIBTESS2_SOURCE_FILES} ${FIXTURE_SOURCE_FILES} ${COMPARISON_SOURCE_FILES})

# set(TESTS_SOURCE_FILES ${COMMON_SOURCE_FILES} test/tap.cpp test/tap.hpp test/test.cpp)
# set(BENCH_SOURCE_FILES ${COMMON_SOURCE_FILES} test/bench.cpp)
# set(VIZ_SOURCE_FILES ${COMMON_SOURCE_FILES} test/viz.cpp)

# if (EARCUT_BUILD_TESTS)
#     add_executable(tests ${TESTS_SOURCE_FILES})
#     target_link_libraries(tests ${Boost_LIBRARIES})
# endif()
# if (EARCUT_BUILD_BENCH)
#     add_executable(bench ${BENCH_SOURCE_FILES})
#     target_link_libraries(bench ${Boost_LIBRARIES})
# endif()
# if (EARCUT_BUILD_VIZ)
#     add_executable(viz ${VIZ_SOURCE_FILES})
#     target_link_libraries(viz ${Boost_LIBRARIES} ${GLFW_LIBS} ${OPENGL_LIBRARIES})
# endif()



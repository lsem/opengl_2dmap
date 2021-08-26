add_library(gg "include/gg/gg.h" "gg.cpp")
set_target_properties(gg PROPERTIES CXX_STANDARD "20")
target_include_directories(gg PUBLIC "include")


# gg_tests
add_executable(gg_tests "gg_tests.cpp")
set_target_properties(gg_tests PROPERTIES CXX_STANDARD "17")
target_link_libraries(gg_tests PRIVATE gg GTest::gtest common fmt::fmt)


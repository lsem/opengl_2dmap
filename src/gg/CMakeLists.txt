add_library(gg "include/gg/gg.h" "gg.cpp")
target_include_directories(gg PUBLIC "include")
target_link_libraries(gg PUBLIC glm)

add_executable(gg_tests "gg_tests.cpp")
target_link_libraries(gg_tests PRIVATE gg GTest::gtest common fmt::fmt)



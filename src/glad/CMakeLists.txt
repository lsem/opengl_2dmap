
add_library(glad "glad.c" "include/glad/glad.h")
set_target_properties(glad PROPERTIES CXX_STANDARD "20")
target_include_directories(glad PUBLIC "include")
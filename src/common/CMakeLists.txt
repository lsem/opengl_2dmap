file(GLOB_RECURSE COMMON_H_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE COMMON_CPP_FILES CONFIGURE_DEPENDS "*.cpp")
add_library(common ${COMMON_H_FILES} ${COMMON_CPP_FILES})
target_link_libraries(common PUBLIC fmt::fmt glfw glm gg glad)
target_include_directories(common PUBLIC "include")
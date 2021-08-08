hunter_add_package(Microsoft.GSL)
find_package(Microsoft.GSL CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE COMMON_H_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE COMMON_CPP_FILES CONFIGURE_DEPENDS "*.cpp")
add_library(common ${COMMON_H_FILES} ${COMMON_CPP_FILES})
set_target_properties(common PROPERTIES CXX_STANDARD "20")
target_link_libraries(common PUBLIC fmt::fmt glfw glm gg Microsoft.GSL::GSL)
target_include_directories(common PUBLIC "include")
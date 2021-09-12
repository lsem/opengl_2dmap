hunter_add_package(glfw)
find_package(glfw3 REQUIRED)

hunter_add_package(glm)
find_package(glm REQUIRED)

file(GLOB_RECURSE RENDERLIB_H_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE RENDERLIB_CPP_FILES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE RENDERLIB_GLSL_FILES CONFIGURE_DEPENDS "*.glsl")
source_group("GlSl" FILES ${RENDERLIB_GLSL_FILES})

add_library(render_lib ${RENDERLIB_H_FILES} ${RENDERLIB_CPP_FILES} ${RENDERLIB_GLSL_FILES})
set_target_properties(render_lib PROPERTIES CXX_STANDARD "17")
target_link_libraries(render_lib PRIVATE glfw glm common glad dear_imgui)
target_include_directories(render_lib PUBLIC "include")
target_include_directories(render_lib PUBLIC ".")

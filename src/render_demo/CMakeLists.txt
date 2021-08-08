
# render_demo
file(GLOB_RECURSE RENDERDEMO_H_FILES  CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE RENDERDEMO_CPP_FILES  CONFIGURE_DEPENDS "*.cpp")
add_executable(render_demo ${RENDERDEMO_H_FILES} ${RENDERDEMO_CPP_FILES})
# target_include_directories(render_demo PRIVATE "include/")
# target_include_directories(render_demo PRIVATE ".")
set_target_properties(render_demo PROPERTIES CXX_STANDARD "17")
target_link_libraries(render_demo PRIVATE glfw common render_lib glad)

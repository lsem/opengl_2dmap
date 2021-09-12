file(GLOB_RECURSE H_FILES  CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE CPP_FILES  CONFIGURE_DEPENDS "*.cpp")
add_executable(render_demo ${H_FILES} ${CPP_FILES})
target_include_directories(render_demo PRIVATE ".")
target_link_libraries(render_demo PRIVATE glfw common render_lib glad mapbox_earcut dear_imgui)
target_link_libraries(render_demo PRIVATE map_compiler)


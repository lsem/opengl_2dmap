# render_demo
file(GLOB_RECURSE RENDERDEMO_H_FILES  CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE RENDERDEMO_CPP_FILES  CONFIGURE_DEPENDS "*.cpp")
#add_executable(map_compiler ${RENDERDEMO_H_FILES} ${RENDERDEMO_CPP_FILES})
add_library(map_compiler ${RENDERDEMO_H_FILES} ${RENDERDEMO_CPP_FILES})
target_include_directories(map_compiler PUBLIC ".")
set_target_properties(map_compiler PROPERTIES CXX_STANDARD "17")
target_link_libraries(map_compiler PRIVATE common shapelib gg)


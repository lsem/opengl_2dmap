file(GLOB_RECURSE H_FILES  CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE CPP_FILES  CONFIGURE_DEPENDS "*.cpp")
add_library(map_compiler ${H_FILES} ${CPP_FILES})
target_include_directories(map_compiler PUBLIC ".")
target_link_libraries(map_compiler PRIVATE common shapelib gg)

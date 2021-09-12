
hunter_add_package(glfw)
find_package(glfw3 REQUIRED)


file(GLOB_RECURSE H_FILES  CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE CPP_FILES  CONFIGURE_DEPENDS "*.cpp")

add_library(dear_imgui ${H_FILES} ${CPP_FILES})
target_include_directories(dear_imgui PUBLIC ".")
set_target_properties(dear_imgui PROPERTIES CXX_STANDARD "17")
target_link_libraries(dear_imgui PUBLIC glad glfw)
target_compile_definitions(dear_imgui PUBLIC "-DIMGUI_IMPL_OPENGL_LOADER_GLAD -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS")



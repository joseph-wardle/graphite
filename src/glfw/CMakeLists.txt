CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
            "GLFW_BUILD_TESTS OFF"
            "GLFW_BUILD_EXAMPLES OFF"
            "GLFW_BUILD_DOCS OFF"
            "BUILD_SHARED_LIBS OFF"
)

add_library(graphite_glfw STATIC)
add_library(graphite::glfw ALIAS graphite_glfw)

target_sources(graphite_glfw
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            graphite.glfw.cppm
            graphite.glfw.window.cppm
            graphite.glfw.input.cppm
    PRIVATE
        graphite.glfw.window.cpp
        graphite.glfw.input.cpp
)

target_link_libraries(graphite_glfw PUBLIC carbon PRIVATE glfw)
target_compile_features(graphite_glfw PUBLIC cxx_std_23)
graphite_strict_warnings(graphite_glfw)

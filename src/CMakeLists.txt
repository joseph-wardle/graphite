option(GRAPHITE_VULKAN "Include Vulkan" ON)
option(GRAPHITE_GLFW   "Include GLFW"   ON)

CPMAddPackage(
        NAME carbon
        GITHUB_REPOSITORY joseph-wardle/carbon
        GIT_TAG main
        OPTIONS "CARBON_TEST OFF"
)

add_library(graphite INTERFACE)
add_library(graphite::graphite ALIAS graphite)

if(GRAPHITE_GLFW)
    add_subdirectory(glfw)
    target_link_libraries(graphite INTERFACE graphite::glfw)
endif()

if(GRAPHITE_VULKAN)
    add_subdirectory(vulkan)
    target_link_libraries(graphite INTERFACE graphite::vulkan)
endif()

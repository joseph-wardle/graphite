CPMAddPackage(
        NAME VulkanHeaders
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
        VERSION 1.4.304
)

CPMAddPackage(
        NAME volk
        GITHUB_REPOSITORY zeux/volk
        GIT_TAG 1.4.304
)

if (TARGET volk AND TARGET Vulkan::Headers)
    target_link_libraries(volk PRIVATE Vulkan::Headers)
endif()

add_library(graphite_vulkan STATIC)
add_library(graphite::vulkan ALIAS graphite_vulkan)

target_sources(graphite_vulkan
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            graphite.vulkan.cppm
            graphite.vulkan.memory.cppm
            graphite.vulkan.swapchain.cppm
    PRIVATE
        graphite.vulkan.memory.cpp
        graphite.vulkan.swapchain.cpp
)

target_link_libraries(graphite_vulkan PUBLIC carbon PRIVATE volk Vulkan::Headers)
target_compile_features(graphite_vulkan PUBLIC cxx_std_23)
graphite_strict_warnings(graphite_vulkan)
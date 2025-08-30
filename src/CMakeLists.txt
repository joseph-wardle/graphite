CPMAddPackage(
        NAME carbon
        GITHUB_REPOSITORY joseph-wardle/carbon
        GIT_TAG main
        OPTIONS "CARBON_TEST OFF"
)

add_library(graphite INTERFACE)
add_library(graphite::graphite ALIAS graphite)

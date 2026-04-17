###############
# DEPENDENCIES #
###############

# tinyobjloader
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE third-party/tinyobjloader)

# json
add_library(json INTERFACE)
target_include_directories(json INTERFACE third-party/json)

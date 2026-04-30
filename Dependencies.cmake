###############
# DEPENDENCIES #
###############

# rapidobj
add_library(rapidobj INTERFACE)
target_include_directories(rapidobj INTERFACE third-party/rapidobj)

# json
add_library(json INTERFACE)
target_include_directories(json INTERFACE third-party/json)

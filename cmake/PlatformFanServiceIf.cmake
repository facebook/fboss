# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fan_config_structs_types_cpp2
  fboss/platform/fan_service/if/fan_config_structs.thrift
  OPTIONS
    json
    reflection
)

# Make to build libraries and binaries in fboss/led_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  led_structs_types_cpp2
  fboss/led_service/if/led_structs.thrift
  OPTIONS
    json
    reflection
)

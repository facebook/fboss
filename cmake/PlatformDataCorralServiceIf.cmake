# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  data_corral_service_cpp2
  fboss/platform/data_corral_service/if/data_corral_service.thrift
  SERVICES
    DataCorralServiceThrift
  OPTIONS
    json
    reflection
  DEPENDS
    fb303_cpp2
    fboss_cpp2
)

# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  misc_service_cpp2
  fboss/platform/misc_service/if/misc_service.thrift
  SERVICES
    MiscServiceThrift
  OPTIONS
    json
  DEPENDS
    fb303_cpp2
    fboss_cpp2
)


# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  rackmon_cpp2
  fboss/platform/rackmon/if/rackmonsvc.thrift
  SERVICES
  RackmonCtrl
  OPTIONS
    json
  DEPENDS
    fb303_cpp2
    fboss_cpp2
)

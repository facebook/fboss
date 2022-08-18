# CMake to build libraries and binaries in fboss/agent/capture

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  thriftpath_test_cpp2
  fboss/fsdb/tests/thriftpath_test.thrift
  OPTIONS
    json
    reflection
)

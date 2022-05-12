# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(thrift_storage
  fboss/thrift_storage/Storage.h
)

target_link_libraries(thrift_storage
  fsdb_oper_cpp2
  Folly::folly
)

# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(thrift_visitors
  fboss/thrift_storage/visitors/NameToPathVisitor.h
  fboss/thrift_storage/visitors/ThriftDeltaVisitor.h
  fboss/thrift_storage/visitors/ThriftLeafVisitor.h
  fboss/thrift_storage/visitors/ThriftPathVisitor.h
)

target_link_libraries(thrift_visitors
  Folly::folly
  FBThrift::thriftcpp2
)

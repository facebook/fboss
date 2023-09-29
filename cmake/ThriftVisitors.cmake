# CMake to build libraries and binaries in fboss/agent/capture

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(
  thrift_visitors
  fboss/thrift_visitors/NameToPathVisitor.h
  fboss/thrift_visitors/ThriftDeltaVisitor.h
  fboss/thrift_visitors/ThriftLeafVisitor.h
  fboss/thrift_visitors/ThriftPathVisitor.h
)

target_compile_options(thrift_visitors PUBLIC "-ftemplate-backtrace-limit=0")

target_link_libraries(thrift_visitors
  fsdb_oper_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

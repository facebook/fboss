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

add_library(fsdb_test_server
  fboss/fsdb/tests/utils/FsdbTestServer.h
  fboss/fsdb/tests/utils/FsdbTestServer.cpp
  fboss/fsdb/tests/utils/oss/FsdbTestServer.cpp
)

target_link_libraries(fsdb_test_server
  fsdb_handler
  fsdb_oper
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(fsdb_test_subscriber
  fboss/fsdb/tests/utils/FsdbTestSubscriber.h
)

target_link_libraries(fsdb_test_subscriber
  fsdb_pub_sub
  fsdb_model_cpp2
  Folly::folly
)

add_executable(fsdb_utils_benchmark
  fboss/fsdb/tests/utils/FsdbUtilsBenchmark.cpp
)

target_link_libraries(fsdb_utils_benchmark
  fsdb_common
  Folly::folly
  ${GTEST}
  ${LIBGMOCK}
  ${GFLAGS}
)

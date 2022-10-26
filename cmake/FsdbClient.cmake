# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include_directories(
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

add_library(fsdb_stream_client
  fboss/fsdb/client/FsdbStreamClient.cpp
  fboss/fsdb/client/oss/FsdbStreamClient.cpp
)

target_link_libraries(fsdb_stream_client
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(fsdb_pub_sub
  fboss/fsdb/client/FsdbSubscriber.cpp
  fboss/fsdb/client/FsdbPublisher.cpp
  fboss/fsdb/client/FsdbPubSubManager.cpp
)

target_link_libraries(fsdb_pub_sub
  fsdb_common_cpp2
  fsdb_flags
  fsdb_oper_cpp2
  fsdb_stream_client
  Folly::folly
  FBThrift::thriftcpp2
  fb303::fb303
)

add_library(fsdb_syncer
  fboss/fsdb/client/FsdbSyncManager.h
)

target_link_libraries(fsdb_syncer
  cow_storage_mgr
  fsdb_common_cpp2
  fsdb_flags
  fsdb_oper_cpp2
  fsdb_stream_client
  fsdb_pub_sub
  Folly::folly
  FBThrift::thriftcpp2
  fb303::fb303
)

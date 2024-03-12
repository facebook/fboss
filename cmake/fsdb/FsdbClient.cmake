# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include_directories(
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

if (FBOSS_CENTOS9)
add_library(fsdb_client
  fboss/fsdb/client/Client.cpp
)

target_link_libraries(fsdb_client
  fsdb_cpp2
  Folly::folly
  thrift_service_client
)
endif()

add_library(fsdb_stream_client
  fboss/fsdb/client/FsdbStreamClient.cpp
  fboss/fsdb/client/oss/FsdbStreamClient.cpp
)

set(fsdb_stream_client_libs
  fsdb_utils
  Folly::folly
  FBThrift::thriftcpp2
  common_thrift_utils
)

if (FBOSS_CENTOS9)
  list(APPEND fsdb_stream_client_libs fsdb_oper_cpp2 fsdb_cpp2 fsdb_client)
endif()

target_link_libraries(fsdb_stream_client ${fsdb_stream_client_libs})

set(fsdb_pub_sub_files
  fboss/fsdb/client/FsdbSubscriber.cpp
  fboss/fsdb/client/FsdbPublisher.cpp
  fboss/fsdb/client/FsdbPubSubManager.cpp
)

if (FBOSS_CENTOS9)
  list(APPEND fsdb_pub_sub_files
    fboss/fsdb/client/FsdbDeltaPublisher.cpp
    fboss/fsdb/client/FsdbDeltaSubscriber.cpp
    fboss/fsdb/client/FsdbStatePublisher.cpp
    fboss/fsdb/client/FsdbStateSubscriber.cpp
  )
endif()

add_library(fsdb_pub_sub ${fsdb_pub_sub_files})

set(fsdb_pub_sub_libs
  fsdb_common_cpp2
  fsdb_flags
  fsdb_oper_cpp2
  fsdb_stream_client
  Folly::folly
  FBThrift::thriftcpp2
  fb303::fb303
)

if (FBOSS_CENTOS9)
  list(APPEND fsdb_pub_sub_libs fsdb_cpp2)
endif()

target_link_libraries(fsdb_pub_sub ${fsdb_pub_sub_libs})

add_library(fsdb_syncer
  fboss/fsdb/client/FsdbSyncManager.h
  fboss/fsdb/client/FsdbSyncManager.cpp
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

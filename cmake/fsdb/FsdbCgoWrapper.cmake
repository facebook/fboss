# CMake to build FsdbCgoPubSubWrapper library for CGO (Go<->C++ interop)

include_directories(
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# FsdbCgoPubSubWrapper library
add_library(fsdb_cgo_pub_sub_wrapper
  fboss/fsdb/client/cgo/FsdbCgoPubSubWrapper.cpp
)

set(fsdb_cgo_pub_sub_wrapper_libs
  fsdb_pub_sub
  fsdb_model
  thrift_service_client
  Folly::folly
  FBThrift::thriftcpp2
)

target_link_libraries(fsdb_cgo_pub_sub_wrapper ${fsdb_cgo_pub_sub_wrapper_libs})

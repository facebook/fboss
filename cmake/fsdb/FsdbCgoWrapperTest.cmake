# CMake to build FsdbCgoPubSubWrapper tests

include_directories(
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

add_executable(fsdb_cgo_wrapper_test
  fboss/agent/test/oss/Main.cpp
  fboss/fsdb/client/cgo/test/FsdbCgoWrapperTest.cpp
)

target_link_libraries(fsdb_cgo_wrapper_test
  fsdb_cgo_pub_sub_wrapper
  fsdb_test_server
  fsdb_test_clients
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(fsdb_cgo_wrapper_test)

# Register this executable for fsdb_all_services target
set(FSDB_EXECUTABLES ${FSDB_EXECUTABLES} fsdb_cgo_wrapper_test CACHE INTERNAL "List of all FSDB executables")

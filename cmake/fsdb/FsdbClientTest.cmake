add_executable(fsdb_client_test
  fboss/agent/test/oss/Main.cpp
  fboss/fsdb/client/test/FsdbPubSubManagerTest.cpp
  fboss/fsdb/client/test/FsdbStreamClientTest.cpp
  fboss/fsdb/client/test/FsdbPublisherTest.cpp
)

target_link_libraries(fsdb_client_test
  fsdb_pub_sub
  fsdb_model
  fsdb_test_clients
  error
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(fsdb_client_test)

# Register this executable for fsdb_all_services target
set(FSDB_EXECUTABLES ${FSDB_EXECUTABLES} fsdb_client_test CACHE INTERNAL "List of all FSDB executables")

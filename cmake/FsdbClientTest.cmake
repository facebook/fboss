add_executable(fsdb_client_test
  fboss/agent/test/oss/Main.cpp
  fboss/fsdb/client/test/FsdbPubSubManagerTest.cpp
  fboss/fsdb/client/test/FsdbStreamClientTest.cpp
  fboss/fsdb/client/test/FsdbPublisherTest.cpp
)

target_link_libraries(fsdb_client_test
  fsdb_pub_sub
  error
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(fsdb_client_test)

